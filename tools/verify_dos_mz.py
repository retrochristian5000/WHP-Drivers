#!/usr/bin/env python3
"""Structural DOS MZ EXE preflight; does not execute the program."""
from __future__ import annotations
import argparse
from pathlib import Path
import struct

def verify_mz(data: bytes) -> None:
    if len(data) < 28 or data[:2] != b"MZ":
        raise ValueError("missing DOS MZ header")
    last_page_bytes, pages, reloc_count, header_paragraphs = struct.unpack_from(
        "<4H", data, 2
    )
    reloc_table_offset = struct.unpack_from("<H", data, 0x18)[0]
    if pages == 0 or last_page_bytes > 511:
        raise ValueError("invalid DOS MZ page count")
    expected = (pages - 1) * 512 + (last_page_bytes or 512)
    if expected > len(data):
        raise ValueError("truncated DOS MZ image")
    header_size = header_paragraphs * 16
    if header_size < 28 or header_size >= expected:
        raise ValueError("MZ header overlaps or consumes entire image")
    if reloc_count and (
        reloc_table_offset < 28 or reloc_table_offset + reloc_count * 4 > header_size
    ):
        raise ValueError("MZ relocation table outside header")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("file", type=Path)
    args = parser.parse_args()
    try:
        verify_mz(args.file.read_bytes())
    except (OSError, ValueError) as exc:
        parser.error(str(exc))
    print("WHPPCI: DOS MZ structure accepted (not a guest runtime test)")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
