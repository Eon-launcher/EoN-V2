#!/usr/bin/env python3
"""Inventory selected Itanium RTTI names and RTTI-backed vtable candidates."""

import argparse
import json
import mmap
import re
import struct
import subprocess
from pathlib import Path


CLASS_NAMES = (
    "LocalPlayer",
    "Player",
    "Actor",
    "Mob",
    "Level",
    "ClientInstance",
    "LevelRenderer",
    "BlockSource",
    "Dimension",
    "LevelChunk",
    "LevelRendererPlayer",
    "Chunk",
    "SubChunk",
    "ChunkSource",
)
RELATIVE_RELOCATION = 1027


def c_string(blob, offset):
    end = blob.find(b"\0", offset)
    if end < 0:
        end = len(blob)
    return blob[offset:end].decode("utf-8", errors="replace")


def sections_from_elf(blob):
    if blob[:4] != b"\x7fELF" or blob[4] != 2 or blob[5] != 1:
        raise ValueError("expected a little-endian ELF64 file")
    shoff = struct.unpack_from("<Q", blob, 40)[0]
    shentsize, shnum, shstrndx = struct.unpack_from("<HHH", blob, 58)
    raw_sections = []
    for index in range(shnum):
        raw_sections.append(
            struct.unpack_from("<IIQQQQIIQQ", blob, shoff + index * shentsize)
        )
    names_section = raw_sections[shstrndx]
    names = blob[names_section[4] : names_section[4] + names_section[5]]
    result = {}
    for section in raw_sections:
        name = c_string(names, section[0])
        result[name] = {
            "type": section[1],
            "address": section[3],
            "offset": section[4],
            "size": section[5],
            "entry_size": section[9],
        }
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("library", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--classes-only", action="store_true")
    args = parser.parse_args()

    with args.library.open("rb") as stream:
        with mmap.mmap(stream.fileno(), 0, access=mmap.ACCESS_READ) as blob:
            sections = sections_from_elf(blob)
            rodata = sections[".rodata"]
            relas = sections[".rela.dyn"]
            relocs_by_target = {}
            relocs_by_place = {}
            for offset in range(
                relas["offset"], relas["offset"] + relas["size"], 24
            ):
                place, info, addend = struct.unpack_from("<QQq", blob, offset)
                if (info & 0xFFFFFFFF) == RELATIVE_RELOCATION and addend >= 0:
                    relocs_by_target.setdefault(addend, []).append(place)
                    relocs_by_place[place] = addend

            executable_ranges = [
                (section["address"], section["address"] + section["size"])
                for name, section in sections.items()
                if name in (".text", "malloc_hook", ".plt")
            ]
            classes = []
            for name in CLASS_NAMES:
                encoded = f"{len(name)}{name}".encode("ascii") + b"\0"
                search_at = rodata["offset"]
                search_end = search_at + rodata["size"]
                typeinfos = {}
                while True:
                    found = blob.find(encoded, search_at, search_end)
                    if found < 0:
                        break
                    name_va = rodata["address"] + found - rodata["offset"]
                    for name_reloc in relocs_by_target.get(name_va, []):
                        typeinfo = name_reloc - 8
                        typeinfos[typeinfo] = {
                            "name_address": name_va,
                            "name_pointer_relocation": name_reloc,
                        }
                    search_at = found + 1

                records = []
                data_rel_ro = sections[".data.rel.ro"]
                for typeinfo, evidence in sorted(typeinfos.items()):
                    candidates = []
                    for rtti_reloc in relocs_by_target.get(typeinfo, []):
                        if not (
                            data_rel_ro["address"] + 8
                            <= rtti_reloc
                            < data_rel_ro["address"] + data_rel_ro["size"]
                        ):
                            continue
                        table = rtti_reloc - 8
                        first_slot = rtti_reloc + 8
                        slot0 = relocs_by_place.get(first_slot)
                        if slot0 is not None and not any(
                            start <= slot0 < end for start, end in executable_ranges
                        ):
                            slot0 = None
                        table_file_offset = (
                            data_rel_ro["offset"]
                            + table
                            - data_rel_ro["address"]
                        )
                        offset_to_top = struct.unpack_from(
                            "<q", blob, table_file_offset
                        )[0]
                        candidates.append(
                            {
                                "address": f"0x{table:x}",
                                "rtti_pointer_location": f"0x{rtti_reloc:x}",
                                "offset_to_top": offset_to_top,
                                "slot0_function": (
                                    f"0x{slot0:x}" if slot0 is not None else None
                                ),
                                "confidence": "LIKELY",
                            }
                        )
                    records.append(
                        {
                            "address": f"0x{typeinfo:x}",
                            "name_address": f"0x{evidence['name_address']:x}",
                            "name_pointer_relocation": (
                                f"0x{evidence['name_pointer_relocation']:x}"
                            ),
                            "confidence": "CONFIRMED",
                            "vtable_candidates": candidates,
                        }
                    )

                classes.append(
                    {
                        "class": name,
                        "typeinfo": records,
                        "status": "found" if records else "not recovered",
                    }
                )

            readelf = subprocess.run(
                ["readelf", "-W", "-n", str(args.library)],
                check=True,
                capture_output=True,
                text=True,
            ).stdout
            build_id_match = re.search(r"Build ID: ([0-9a-f]+)", readelf)
            if not build_id_match:
                raise ValueError("GNU build ID was not present")

            nm = subprocess.run(
                ["nm", "-D", "--defined-only", str(args.library)],
                check=True,
                capture_output=True,
                text=True,
            ).stdout.splitlines()

            report = {
                "library": args.library.name,
                "architecture": "AArch64 ELF64 little-endian",
                "build_id": build_id_match.group(1),
                "section_count": len(sections),
                "sections": sections,
                "defined_dynamic_symbol_count": len(nm),
                "classes": classes,
                "notes": [
                    "Vtable candidates are RTTI-pointer locations with a preceding "
                    "two-word Itanium ABI prefix; verify table extent and class role.",
                    "Missing classes were not assigned addresses.",
                ],
            }
            if args.classes_only:
                report = {
                    "library": report["library"],
                    "architecture": report["architecture"],
                    "build_id": report["build_id"],
                    "classes": report["classes"],
                    "notes": report["notes"],
                }
            encoded = json.dumps(report, indent=2) + "\n"
            if args.output:
                args.output.write_text(encoded, encoding="utf-8")
            else:
                print(encoded, end="")


if __name__ == "__main__":
    main()
