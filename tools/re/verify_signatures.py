#!/usr/bin/env python3
"""Verify exact-once masked byte signatures against a matching ELF build."""

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path


CONFIDENCE = {"CONFIRMED", "LIKELY", "GUESS"}


def parse_pattern(signature):
    tokens = signature.split()
    if not tokens:
        raise ValueError("signature is empty")
    pattern = []
    for token in tokens:
        if token == "??":
            pattern.append(None)
        elif re.fullmatch(r"[0-9a-fA-F]{2}", token):
            pattern.append(int(token, 16))
        else:
            raise ValueError(f"invalid pattern byte: {token}")
    if all(byte is None for byte in pattern):
        raise ValueError("signature must contain at least one fixed byte")
    return pattern


def find_matches(data, pattern):
    runs = []
    index = 0
    while index < len(pattern):
        if pattern[index] is None:
            index += 1
            continue
        start = index
        run = bytearray()
        while index < len(pattern) and pattern[index] is not None:
            run.append(pattern[index])
            index += 1
        runs.append((start, bytes(run)))
    anchor_start, anchor = max(runs, key=lambda item: len(item[1]))
    matches = []
    cursor = 0
    while True:
        found = data.find(anchor, cursor)
        if found < 0:
            break
        candidate = found - anchor_start
        if candidate >= 0 and candidate + len(pattern) <= len(data):
            if all(
                expected is None or data[candidate + offset] == expected
                for offset, expected in enumerate(pattern)
            ):
                matches.append(candidate)
        cursor = found + 1
    return matches


def build_id(library):
    result = subprocess.run(
        ["readelf", "-W", "-n", str(library)],
        check=True,
        capture_output=True,
        text=True,
    )
    match = re.search(r"Build ID: ([0-9a-f]+)", result.stdout)
    if not match:
        raise ValueError("GNU build ID was not present")
    return match.group(1)


def parse_offset(value):
    if isinstance(value, int):
        return value
    if isinstance(value, str):
        return int(value, 0)
    raise ValueError("match_offset must be an integer or hex string")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("library", type=Path)
    parser.add_argument("signatures_json", type=Path)
    args = parser.parse_args()

    report = json.loads(args.signatures_json.read_text(encoding="utf-8"))
    if not isinstance(report, dict):
        print("ERROR: signatures JSON must contain an object", file=sys.stderr)
        return 1
    actual_build_id = build_id(args.library)
    if actual_build_id != report.get("build_id"):
        print(
            f"ERROR: build ID mismatch: expected {report.get('build_id')}, "
            f"got {actual_build_id}",
            file=sys.stderr,
        )
        return 1

    signatures = report.get("signatures")
    if not isinstance(signatures, list):
        print("ERROR: signatures must be a JSON array", file=sys.stderr)
        return 1
    if not signatures:
        print(
            "BLOCKED: no verified signatures are available to check",
            file=sys.stderr,
        )
        return 2

    import mmap

    failures = 0
    with args.library.open("rb") as stream:
        with mmap.mmap(stream.fileno(), 0, access=mmap.ACCESS_READ) as data:
            for entry in signatures:
                name = (
                    entry.get("name", "<unnamed>")
                    if isinstance(entry, dict)
                    else "<invalid>"
                )
                try:
                    if not isinstance(entry, dict):
                        raise ValueError("entry must be a JSON object")
                    required = (
                        "name",
                        "signature",
                        "match_offset",
                        "match_count",
                        "confidence",
                        "notes",
                    )
                    if any(field not in entry for field in required):
                        raise ValueError("entry is missing required fields")
                    if entry["confidence"] not in CONFIDENCE:
                        raise ValueError("invalid confidence label")
                    expected_count = entry["match_count"]
                    if not isinstance(expected_count, int) or expected_count < 0:
                        raise ValueError("match_count must be a nonnegative integer")
                    expected_offset = parse_offset(entry["match_offset"])
                    matches = find_matches(data, parse_pattern(entry["signature"]))
                    if len(matches) != expected_count or len(matches) != 1:
                        raise ValueError(
                            f"expected exactly one match (declared "
                            f"{expected_count}), found {len(matches)}"
                        )
                    if matches[0] != expected_offset:
                        raise ValueError(
                            f"expected offset {expected_offset:#x}, found "
                            f"{matches[0]:#x}"
                        )
                    print(f"PASS {entry['name']}: {matches[0]:#x} (1 match)")
                except (KeyError, TypeError, ValueError) as error:
                    failures += 1
                    print(
                        f"FAIL {name}: {error}",
                        file=sys.stderr,
                    )

    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
