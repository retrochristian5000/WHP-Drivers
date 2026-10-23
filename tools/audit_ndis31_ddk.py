#!/usr/bin/env python3
"""Read-only, fail-closed provenance preflight for an independently sourced
Windows NT 3.51 DDK include tree. Hash pins prove byte identity ONLY.
Neither filename nor hash equality authenticates a kit or validates its ABI.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import re
import sys

HEADERS = ("ndis.h", "ntddk.h")
# These are later NDIS surfaces, not authorized for the Win95 RTM miniport.
LATER_TOKENS = (
    "NDIS_MINIPORT_DRIVER_CHARACTERISTICS",
    "NdisMRegisterMiniportDriver",
    "NdisMSetMiniportAttributes",
    "NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES",
    "NET_BUFFER_LIST",
    "NdisMRegisterInterruptEx",
    "MiniportSendPackets",
    "NDIS_OBJECT_HEADER",
)
HISTORICAL_LEADS = (
    "NDIS_MINIPORT_CHARACTERISTICS",
    "NdisMRegisterMiniport",
    "NdisMInitializeWrapper",
)


class DDKAuditError(ValueError):
    """DDK source set cannot pass the requested preflight."""


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def load(include: Path) -> dict[str, bytes]:
    if not include.is_dir():
        raise DDKAuditError(f"DDK include directory does not exist: {include}")
    result = {}
    for name in HEADERS:
        path = include / name
        if not path.is_file():
            raise DDKAuditError(f"missing historical DDK header: {path}")
        if path.is_symlink():
            raise DDKAuditError(f"symlinked DDK header is not accepted: {path}")
        data = path.read_bytes()
        if not data or len(data) > 2 * 1024 * 1024:
            raise DDKAuditError(f"empty or oversized DDK header: {path}")
        result[name] = data
    return result


def code_without_comments(data: bytes) -> str:
    # This is a diagnostic scan, not a C preprocessor or full parser.
    # Compile with the original DDK after preflight for real type checking.
    text = data.decode("latin-1")
    return re.sub(r"/\*.*?\*/|//[^\r\n]*", " ", text, flags=re.DOTALL)


def inspect(include: Path) -> tuple[dict[str, str], tuple[str, ...]]:
    files = load(include)
    hashes = {name: sha256(data) for name, data in files.items()}
    source = "\n".join(code_without_comments(data) for data in files.values())
    found = tuple(token for token in LATER_TOKENS if re.search(r"\b" + re.escape(token) + r"\b", source))
    print("WHP-Drivers: NT 3.51 DDK candidate inventory (NOT authentication)")
    for name in HEADERS:
        print(f"  {name}: SHA256={hashes[name]}")
    print("  classic declarations (diagnostic only):")
    for name in HISTORICAL_LEADS:
        present = bool(re.search(r"\b" + re.escape(name) + r"\b", source))
        print(f"    {name}: {'present' if present else 'not located'}")
    if found:
        print("  later-version NDIS identifiers: " + ", ".join(found))
    else:
        print("  later-version NDIS identifiers: none detected in these two files")
    return hashes, found


def verify(include: Path, pins: dict[str, str]) -> None:
    for name in HEADERS:
        expected = pins[name]
        if not re.fullmatch(r"[0-9a-fA-F]{64}", expected):
            raise DDKAuditError(f"invalid SHA256 pin for {name}")
    hashes, later = inspect(include)
    for name in HEADERS:
        if hashes[name] != pins[name].lower():
            raise DDKAuditError(f"hash mismatch for {name}: wrong or changed DDK header")
    if later:
        raise DDKAuditError("later-version NDIS API identifier in the supplied DDK headers")
    print("WHP-Drivers: DDK header SHA256 pins matched; provenance and ABI remain unverified")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("inspect", "verify"))
    parser.add_argument("include", type=Path, help="independently sourced NT 3.51 DDK include directory")
    parser.add_argument("--ndis-sha256", help="independently recorded SHA256 of historical ndis.h")
    parser.add_argument("--ntddk-sha256", help="independently recorded SHA256 of historical ntddk.h")
    args = parser.parse_args(argv)
    try:
        if args.mode == "inspect":
            if args.ndis_sha256 or args.ntddk_sha256:
                raise DDKAuditError("pins apply only to verify mode")
            inspect(args.include)
        else:
            if not args.ndis_sha256 or not args.ntddk_sha256:
                raise DDKAuditError("verify requires BOTH historical DDK header SHA256 pins")
            verify(args.include, {"ndis.h": args.ndis_sha256, "ntddk.h": args.ntddk_sha256})
    except (DDKAuditError, OSError) as exc:
        print(f"WHP-Drivers: NDIS 3.1 DDK preflight rejected: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
