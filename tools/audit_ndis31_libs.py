#!/usr/bin/env python3
"""Read-only i386 COFF/NDIS 3.1 library inventory and unresolved-symbol audit.

No linker is invoked and no output files are created. This does not
authenticate an NT 3.51 DDK or prove the Win95 RTM NDIS loader will accept
a binary. Never substitute modern import libraries merely to satisfy symbols.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import re
import subprocess
import sys

ARCHIVE_MAGIC = b"!<arch>\n"
MAX_LIB_SIZE = 256 * 1024 * 1024
SHA256_RE = re.compile(r"[0-9a-fA-F]{64}\Z")
NDIS_SYMBOL_RE = re.compile(r"Ndis[A-Za-z0-9_]*")


class AuditError(ValueError):
    pass


def checked_regular_file(path: Path) -> None:
    if path.is_symlink() or not path.is_file():
        raise AuditError(f"missing, nonregular or symlinked input: {path}")


def digest(path: Path) -> str:
    checked_regular_file(path)
    hasher = hashlib.sha256()
    with path.open("rb") as src:
        while True:
            data = src.read(65536)
            if not data:
                break
            hasher.update(data)
    return hasher.hexdigest()


def library(path: Path) -> str:
    checked_regular_file(path)
    if path.suffix.lower() != ".lib":
        raise AuditError(f"not an explicit .lib input: {path}")
    size = path.stat().st_size
    if size < len(ARCHIVE_MAGIC) or size > MAX_LIB_SIZE:
        raise AuditError(f"invalid or oversized COFF archive: {path}")
    with path.open("rb") as src:
        if src.read(len(ARCHIVE_MAGIC)) != ARCHIVE_MAGIC:
            raise AuditError(f"unsupported library archive format (not COFF): {path}")
    return digest(path)


def object_file(path: Path) -> str:
    checked_regular_file(path)
    if path.suffix.lower() != ".obj":
        raise AuditError(f"not an explicit .obj input: {path}")
    with path.open("rb") as src:
        first = src.read(20)
    if len(first) < 20 or first[:2] != b"\x4c\x01":
        raise AuditError(f"object is not standard i386 COFF: {path}")
    return digest(path)


def pins_from(values: list[str]) -> dict[str, str]:
    pins: dict[str, str] = {}
    for item in values:
        if "=" not in item:
            raise AuditError(f"expected LIBNAME=SHA256 pin: {item}")
        name, checksum = item.split("=", 1)
        key = name.lower()
        if (not key.endswith(".lib") or Path(key).name != key or
                not SHA256_RE.fullmatch(checksum) or key in pins):
            raise AuditError(f"invalid or duplicate library checksum pin: {item}")
        pins[key] = checksum.lower()
    return pins


def prepare_libraries(paths: list[Path]) -> dict[str, tuple[Path, str]]:
    if not paths:
        raise AuditError("at least one explicit historical .lib is required")
    result: dict[str, tuple[Path, str]] = {}
    for path in paths:
        name = path.name.lower()
        if name in result:
            raise AuditError(f"duplicate/ambiguous library basename: {name}")
        result[name] = (path, library(path))
    return result


def parse_nm(output: str) -> tuple[set[str], set[str]]:
    undefined: set[str] = set()
    defined: set[str] = set()
    for line in output.splitlines():
        line = line.strip()
        if not line or line.endswith(":"):
            continue
        fields = line.split()
        # POSIX -P output: symbol type value [size]. Keep exact decorated
        # x86 symbols; a missing stdcall suffix is an ABI mismatch.
        if len(fields) < 2 or len(fields[1]) != 1 or fields[1] not in "ABCDGIRSTUVWabcdnirstuvw?":
            raise AuditError(f"unrecognized llvm-nm POSIX output: {line[:120]}")
        symbol, kind = fields[0], fields[1]
        if kind == "U":
            undefined.add(symbol)
        elif kind == "w" or kind == "v":
            # Weak undefined references are not mandatory dependencies.
            pass
        elif kind != "?":
            defined.add(symbol)
    return undefined, defined


def symbols(nm: Path, path: Path) -> tuple[set[str], set[str]]:
    try:
        done = subprocess.run(
            [str(nm), "--format=posix", "--extern-only", str(path)],
            check=False, capture_output=True, text=True, timeout=20,
        )
    except (OSError, subprocess.TimeoutExpired) as exc:
        raise AuditError(f"llvm-nm failed for {path}: {exc}") from exc
    if done.returncode:
        raise AuditError(f"llvm-nm rejected {path}: {done.stderr.strip()[:240]}")
    return parse_nm(done.stdout)


def inventory(libs: list[Path]) -> None:
    for name, (path, checksum) in sorted(prepare_libraries(libs).items()):
        print(f"{name}\tSHA256={checksum}\t{path}")
    print("Inventory only; archives not authenticated, linked or executed.")


def resolve(libs: list[Path], objs: list[Path],
            pin_values: list[str], nm: Path) -> None:
    imports = prepare_libraries(libs)
    if "ndis.lib" not in imports:
        raise AuditError("explicit ndis.lib input is missing (miniport dependency)")
    pins = pins_from(pin_values)
    if set(pins) != set(imports):
        raise AuditError("require one independently recorded SHA256 per selected library")
    for name, (_, checksum) in imports.items():
        if pins[name] != checksum:
            raise AuditError(f"historical DDK library checksum mismatch: {name}")
    if not objs:
        raise AuditError("missing miniport .obj inputs")
    checked_regular_file(nm)
    if len(set(map(str, objs))) != len(objs):
        raise AuditError("duplicate object input")
    all_defined: set[str] = set()
    unresolved: set[str] = set()
    for obj in objs:
        obj_checksum = object_file(obj)
        undef, defs = symbols(nm, obj)
        unresolved.update(undef)
        all_defined.update(defs)
        print(f"OBJECT {obj.name} SHA256={obj_checksum}")
    # Fail closed on the current RTL8111E identity/lifecycle-only objects:
    # without an NdisXxx import there is no real NDIS adapter to link.
    if not any(NDIS_SYMBOL_RE.search(name) for name in unresolved):
        raise AuditError("no unresolved NDIS calls: miniport adapter/import contract not yet implemented")
    for name, (path, checksum) in imports.items():
        _, defs = symbols(nm, path)
        all_defined.update(defs)
        print(f"LIBRARY {name} SHA256={checksum}")
    missing = sorted(unresolved - all_defined)
    if missing:
        print("UNRESOLVED external symbols:", file=sys.stderr)
        for name in missing:
            print(f"  {name}", file=sys.stderr)
        raise AuditError("not all object imports are present in selected libraries")
    print("All currently observed undefined symbols have candidates in the")
    print("selected i386 objects/archives. Linker/loader compatibility UNVERIFIED.")
    print("Proposed linker INPUT ORDER (no linker was run):")
    for obj in objs:
        print(f"  OBJECT {obj}")
    for name, (path, _) in imports.items():
        print(f"  LIBRARY {path}")
    print("Do not infer linker flags, subsystem, entry point or image format.")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("inventory", "resolve"))
    parser.add_argument("--lib", type=Path, action="append", default=[])
    parser.add_argument("--obj", type=Path, action="append", default=[])
    parser.add_argument("--pin", action="append", default=[], help="LIBNAME=SHA256")
    parser.add_argument("--nm", type=Path, help="verified LLVM llvm-nm executable")
    args = parser.parse_args(argv)
    try:
        if args.mode == "inventory":
            if args.obj or args.pin or args.nm:
                raise AuditError("inventory accepts only explicit --lib inputs")
            inventory(args.lib)
        else:
            if not args.nm:
                raise AuditError("resolve requires explicit --nm to LLVM llvm-nm")
            resolve(args.lib, args.obj, args.pin, args.nm)
    except (AuditError, OSError) as exc:
        print(f"WHP-Drivers: NDIS 3.1 library audit rejected: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
