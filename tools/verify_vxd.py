#!/usr/bin/env python3
"""Conservative structural preflight for direct-header Windows 9x VxD LE images.

This checks file structure, not VMM runtime compatibility. No output is written.
References: Windows 95 DDK 'Building a Virtual Device' and Geoff Chappell's
VXDLDR LE-header analysis (for WfW 3.11; do not extrapolate its version limits).
"""
import argparse
from pathlib import Path
import struct
import sys


class FormatError(ValueError):
    pass


def require(condition, reason):
    if not condition:
        raise FormatError(reason)


def check_image(data, dynamic=False):
    """Return summary for a plausible direct-header LE VxD or raise FormatError."""
    size = len(data)

    def region(offset, length, label):
        require(0 <= offset <= size and 0 <= length <= size - offset,
                f"{label}: truncated or out of bounds")
        return data[offset:offset + length]

    def u16(offset, label):
        return struct.unpack("<H", region(offset, 2, label))[0]

    def u32(offset, label):
        return struct.unpack("<I", region(offset, 4, label))[0]

    require(region(0, 2, "DOS signature") == b"MZ", "not an MZ-wrapped executable")
    base = u32(0x3C, "e_lfanew")
    require(region(base, 2, "LE signature") == b"LE",
            "not a direct-header LE image (PE, NE, LX and embedded LE are unsupported)")
    region(base, 0xC4, "LE header")
    require(data[base + 2] == 0 and data[base + 3] == 0,
            "non-little-endian LE byte/word ordering")
    cpu = u16(base + 0x08, "LE CPU")
    os_type = u16(base + 0x0A, "LE OS")
    flags = u32(base + 0x10, "LE flags")
    require(cpu >= 2, "VxD requires an 80386-or-newer LE target")
    require(os_type == 4, "LE target OS is not Windows 386 driver")
    if dynamic:
        require((flags & 0x00038000) == 0x00038000,
                "dynamic VxD module flags are missing")
    num_physical = u32(base + 0x14, "physical page count")
    page_size = u32(base + 0x28, "page size")
    last_size = u32(base + 0x2C, "last page size")
    require(page_size != 0 and page_size <= 0x10000 and
            page_size & (page_size - 1) == 0,
            "invalid LE page size")
    require(num_physical == 0 or 0 < last_size <= page_size,
            "invalid last physical page size")
    object_count = u32(base + 0x44, "object count")
    require(0 < object_count <= 0xFFFF, "invalid LE object count")
    object_table = base + u32(base + 0x40, "object table")
    page_map = base + u32(base + 0x48, "page map")
    entry_offset = base + u32(base + 0x5C, "entry table")
    name_offset = base + u32(base + 0x58, "resident name table")
    fixup_pages = base + u32(base + 0x68, "fixup page table")
    fixup_records = base + u32(base + 0x6C, "fixup records")
    physical_start = u32(base + 0x80, "physical data start")
    region(object_table, object_count * 24, "object table")

    logical_count = 0
    objects = []
    for n in range(object_count):
        offset = object_table + 24 * n
        virtual_size = u32(offset, "object size")
        first_page = u32(offset + 12, "object page index")
        page_count = u32(offset + 16, "object page count")
        if page_count:
            require(first_page > 0, "object uses zero-based first page")
            logical_count = max(logical_count, first_page + page_count - 1)
        objects.append(virtual_size)

    require(0 < logical_count <= 0x100000, "invalid logical page count")
    region(page_map, logical_count * 4, "page map")
    for n in range(logical_count):
        entry = region(page_map + 4 * n, 4, "logical page entry")
        physical = int.from_bytes(entry[:3], "big")
        kind = entry[3]
        require(kind in (0, 3), "unsupported LE page type")
        if kind == 0:
            require(1 <= physical <= num_physical, "invalid physical page number")
            length = last_size if physical == num_physical else page_size
            region(physical_start + (physical - 1) * page_size,
                   length, "physical page data")
        else:
            require(physical == 0, "zero-fill page unexpectedly has file data")

    # VxD loader resolves the Device Descriptor Block via export ordinal 1.
    # A 32-bit entry bundle is: count, type, object index, entry flags, offset.
    bundle_count = region(entry_offset, 1, "entry count")[0]
    bundle_type = region(entry_offset + 1, 1, "entry type")[0]
    require(bundle_count > 0 and bundle_type & 0x7F == 3,
            "export ordinal 1 is not a 32-bit entry")
    ddb_object = u16(entry_offset + 2, "DDB object index")
    require(1 <= ddb_object <= object_count, "DDB object index out of bounds")
    region(entry_offset + 4, 1, "DDB entry flags")
    ddb_offset = u32(entry_offset + 5, "DDB entry offset")
    require(ddb_offset < objects[ddb_object - 1], "DDB offset outside object")

    name_length = region(name_offset, 1, "resident name length")[0]
    require(name_length > 0, "empty resident driver name")
    raw_name = region(name_offset + 1, name_length, "resident name")
    require(all(32 <= byte < 127 for byte in raw_name),
            "driver name has non-ASCII or control bytes")
    region(name_offset + 1 + name_length, 2, "resident ordinal")

    # Each LE logical page has two fixup fenceposts, including the final one.
    region(fixup_pages, (logical_count + 1) * 4, "fixup page table")
    offsets = [u32(fixup_pages + 4 * i, "fixup fencepost")
               for i in range(logical_count + 1)]
    require(offsets[0] == 0 and offsets == sorted(offsets),
            "invalid fixup page fenceposts")
    region(fixup_records, offsets[-1], "fixup records")
    for page in range(logical_count):
        pos = fixup_records + offsets[page]
        end = fixup_records + offsets[page + 1]
        while pos < end:
            require(pos + 2 <= end, "truncated fixup header")
            mode = u16(pos, "fixup mode")
            pos += 2
            require((mode & 0x0F) in (7, 8),
                    "unsupported fixup type")
            require((mode & 0x0300) == 0,
                    "external fixup unsupported by this verifier")
            if mode & 0x20:
                require(pos + 1 <= end, "truncated repeated-fixup count")
                count = data[pos]
                pos += 1
                require(count > 0 and pos + count * 2 <= end,
                        "truncated repeated-fixup sources")
                pos += count * 2
            else:
                require(pos + 2 <= end, "truncated fixup source")
                pos += 2
            width = 2 if mode & 0x4000 else 1
            require(pos + width <= end, "truncated fixup target object")
            target = int.from_bytes(data[pos:pos + width], "little")
            require(1 <= target <= object_count,
                    "fixup references invalid target object")
            pos += width
            width = 4 if mode & 0x1000 else 2
            require(pos + width <= end, "truncated fixup target offset")
            target_offset = int.from_bytes(data[pos:pos + width], "little")
            require(target_offset < objects[target - 1],
                    "fixup target offset outside object")
            pos += width
        require(pos == end, "fixup record overran its page")

    return {
        "name": raw_name.decode("ascii"),
        "objects": object_count,
        "logical_pages": logical_count,
        "physical_pages": num_physical,
        "ddb_object": ddb_object,
        "ddb_offset": ddb_offset,
        "fixup_bytes": offsets[-1],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image", type=Path, help="LE VxD image to inspect")
    parser.add_argument("--dynamic", action="store_true",
                        help="require dynamically loadable VxD flags")
    args = parser.parse_args()
    try:
        summary = check_image(args.image.read_bytes(), dynamic=args.dynamic)
    except (OSError, FormatError) as exc:
        print(f"VxD preflight: FAIL: {exc}", file=sys.stderr)
        return 1
    print(f"VxD preflight: PASS (structural only): {summary}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
