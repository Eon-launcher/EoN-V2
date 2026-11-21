#!/usr/bin/env python3
"""Dump RTTI-backed table candidates and bounded AArch64 slot disassembly."""

import argparse
import json
import re
import subprocess
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("library", type=Path)
    parser.add_argument("classes_json", type=Path)
    parser.add_argument("--instructions", type=int, default=40)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.instructions < 1:
        parser.error("--instructions must be positive")

    report = json.loads(args.classes_json.read_text(encoding="utf-8"))
    sections = ["# RTTI-backed vtable candidates", ""]
    sections.extend(
        [
            f"Library build ID: `{report['build_id']}`.",
            "",
            "Table prefixes are **LIKELY** vtables based on the Itanium ABI "
            "offset-to-top/typeinfo prefix and contiguous function relocations. "
            "They may be secondary or construction tables. Slots are numbered "
            "from zero at the first function pointer; no virtual method names "
            "are inferred.",
            "",
        ]
    )
    functions = {}

    for class_record in report["classes"]:
        sections.extend([f"## {class_record['class']}", ""])
        if not class_record["typeinfo"]:
            sections.extend(
                ["No matching RTTI name relocation was recovered.", ""]
            )
            continue
        for typeinfo in class_record["typeinfo"]:
            sections.append(
                f"Typeinfo `{typeinfo['address']}` (CONFIRMED); name VA "
                f"`{typeinfo['name_address']}`."
            )
            for table in typeinfo["vtable_candidates"]:
                slots = table.get("slots", [])
                sections.append(
                    f"- Candidate table `{table['address']}`; "
                    f"offset-to-top `{table['offset_to_top']}`; "
                    f"{len(slots)} contiguous function slots"
                    f"{' (truncated)' if table.get('slots_truncated') else ''}."
                )
                if slots:
                    sections.append("  | Slot | Function VA |")
                    sections.append("  |---:|---:|")
                    for slot in slots:
                        sections.append(
                            f"  | {slot['index']} | `{slot['function']}` |"
                        )
                    functions.setdefault(slots[0]["function"], []).append(
                        f"{class_record['class']} table {table['address']} slot 0"
                    )
            sections.append("")

    sections.extend(
        [
            "## First-slot disassembly",
            "",
            f"The following are the first {args.instructions} AArch64 instructions "
            "at each distinct candidate slot-0 target. No semantic method name "
            "is assigned without a symbol or call-site proof.",
            "",
        ]
    )
    for function, references in sorted(functions.items()):
        address = int(function, 16)
        stop = address + args.instructions * 4
        result = subprocess.run(
            [
                "llvm-objdump",
                "--disassemble",
                "--triple=aarch64-linux-android",
                f"--start-address={address:#x}",
                f"--stop-address={stop:#x}",
                str(args.library),
            ],
            check=True,
            capture_output=True,
            text=True,
        )
        instructions = []
        for line in result.stdout.splitlines():
            if not re.match(r"^\s*[0-9a-f]+:\s+[0-9a-f]+\s+", line):
                continue
            instructions.append(line)
            instruction_match = re.match(
                r"^\s*[0-9a-f]+:\s+[0-9a-f]+\s+([a-z][a-z0-9.]*)", line
            )
            if instruction_match and instruction_match.group(1) in ("ret", "b"):
                break
            if len(instructions) == args.instructions:
                break
        if not instructions:
            raise RuntimeError(f"no AArch64 instructions decoded at {function}")
        sections.extend(
            [
                f"### `{function}`",
                "",
                "Referenced by: " + "; ".join(references) + ".",
                "",
                "```asm",
                "\n".join(instructions),
                "```",
                "",
            ]
        )

    output = "\n".join(sections).rstrip()
    if args.output:
        args.output.write_text(output + "\n", encoding="utf-8")
    else:
        print(output)


if __name__ == "__main__":
    main()
