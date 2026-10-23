#!/usr/bin/env python3
"""Fail-closed i386 COFF and x86 NDIS calling-convention audit.

Parses actual COFF symbols and linker directives; no PE driver is linked.
A synthetic ABI fixture checks LLVM codegen, NOT original NT 3.51 DDK ABI.
"""
from __future__ import annotations
import argparse
import pathlib
import re
import struct
import subprocess
import sys
import tempfile

I386 = 0x14c
MAX_OBJ = 32 * 1024 * 1024
FORBIDDEN_HELPERS = frozenset({
    "chkstk", "chkstk_ms", "security_cookie", "security_check_cookie",
    "security_init_cookie", "memcpy", "memmove", "memset", "malloc",
    "calloc", "free", "udivdi3", "divdi3", "umoddi3", "moddi3", "fltused",
})
FIXTURE = r"""
#if !defined(_M_IX86) && !defined(__i386__)
#error "not an i386 build"
#endif
#if defined(_WIN64)
#error "incorrect Win64 ABI"
#endif
_Static_assert(sizeof(void *) == 4, "pointer ABI");
_Static_assert(sizeof(long) == 4, "Windows ULONG ABI");
/* Deliberately exercise both import-indirect and direct-call variants.
 * DO NOT infer which one an authentic NT 3.51 DDK uses from this fixture. */
#define STD __attribute__((stdcall))
#define CDECL __attribute__((cdecl))
__declspec(dllimport) unsigned STD NdisMRegisterMiniport(void *, void *, unsigned);
extern unsigned STD NdisMInitializeWrapper(void *, void *, void *, void *);
unsigned CDECL whp_ndis_cdecl(unsigned n) { return n + 1u; }
unsigned STD whp_ndis_stdcall(void *p, unsigned n) { return n + (p != 0); }
unsigned CDECL whp_ndis_probe(void *p) {
    return NdisMRegisterMiniport(p, p, 3u) +
           NdisMInitializeWrapper(p, p, p, p);
}
"""
EXPECTED_DEFINED = {"_whp_ndis_cdecl", "_whp_ndis_stdcall@8",
                    "_whp_ndis_probe"}
EXPECTED_UNDEFINED = {"__imp__NdisMRegisterMiniport@12",
                      "_NdisMInitializeWrapper@16"}


class COFFError(ValueError):
    pass


def read_coff(path: pathlib.Path) -> tuple[set[str], set[str]]:
    if path.is_symlink() or not path.is_file():
        raise COFFError(f"missing or symlinked COFF object: {path}")
    if path.stat().st_size > MAX_OBJ:
        raise COFFError(f"oversized COFF object: {path}")
    data = path.read_bytes()
    if len(data) < 20:
        raise COFFError("truncated COFF header")
    machine, sections, _, symtab, count, optional, _ = struct.unpack_from(
        "<HHIIIHH", data, 0)
    if machine != I386 or not 1 <= sections <= 96 or optional:
        raise COFFError("expected standard i386 COFF object (not PE/bigobj/other ABI)")
    if 20 + 40 * sections > len(data):
        raise COFFError("truncated section headers")
    for i in range(sections):
        offset = 20 + 40 * i
        name = data[offset:offset+8].split(b"\0", 1)[0]
        length, ptr = struct.unpack_from("<II", data, offset + 16)
        if length and (ptr < 20 + 40 * sections or
                       ptr > len(data) or length > len(data)-ptr):
            raise COFFError("section data outside object")
        if name == b".drectve":
            directives = data[ptr:ptr+length].decode("latin-1").lower()
            if re.search(r"(?:/|-)(?:defaultlib|failifmismatch|include)\b", directives):
                raise COFFError("implicit default library/forced symbol from .drectve")
        if name.startswith((b".tls", b".CRT")):
            raise COFFError("unexpected TLS/CRT section in legacy miniport object")
    if count < 1 or symtab < 20 + 40 * sections or count > MAX_OBJ//18:
        raise COFFError("missing or invalid COFF symbol table")
    sym_end = symtab + count * 18
    if sym_end + 4 > len(data):
        raise COFFError("truncated COFF symbol/string table")
    strings_size = struct.unpack_from("<I", data, sym_end)[0]
    if strings_size < 4 or strings_size > len(data) - sym_end:
        raise COFFError("invalid COFF long-symbol table")
    strings = data[sym_end:sym_end+strings_size]
    defined: set[str] = set()
    undefined: set[str] = set()
    i = 0
    while i < count:
        offset = symtab + i * 18
        raw, _, section, _, storage, aux = struct.unpack_from(
            "<8sIhHBB", data, offset)
        if i + 1 + aux > count:
            raise COFFError("COFF auxiliary symbol table overflow")
        if raw[:4] == b"\0\0\0\0":
            loc = struct.unpack_from("<I", raw, 4)[0]
            if loc < 4 or loc >= len(strings):
                raise COFFError("invalid long symbol name")
            name_bytes = strings[loc:].split(b"\0", 1)[0]
        else:
            name_bytes = raw.split(b"\0", 1)[0]
        try:
            name = name_bytes.decode("ascii")
        except UnicodeDecodeError as exc:
            raise COFFError("non-ASCII COFF symbol") from exc
        if storage == 2 and name:
            if section == 0:
                undefined.add(name)
            elif section > 0:
                defined.add(name)
        i += 1 + aux
    return defined, undefined


def check_forbidden(undefined: set[str]) -> None:
    def helper_name(name: str) -> str:
        candidate = name.lstrip("_")
        if candidate.lower().startswith("imp_"):
            candidate = candidate[4:].lstrip("_")
        return re.sub(r"@\d+$", "", candidate).lower()

    unexpected = sorted(name for name in undefined
                        if helper_name(name) in FORBIDDEN_HELPERS)
    if unexpected:
        raise COFFError("unverified CRT/stack/security helper imports: " +
                        ", ".join(unexpected))


def audit_core(objects: list[pathlib.Path]) -> None:
    if not objects:
        raise COFFError("no miniport core objects supplied")
    defined: set[str] = set()
    undefined: set[str] = set()
    for path in objects:
        local_defs, local_undef = read_coff(path)
        defined.update(local_defs)
        undefined.update(local_undef)
        print(f"i386 COFF: {path.name} ({len(local_defs)} exports; "
              f"{len(local_undef)} imports)")
    check_forbidden(undefined)
    unresolved = sorted(undefined - defined)
    if unresolved:
        raise COFFError("unresolved core references: " + ", ".join(unresolved))
    print("Portable core is internally symbol-complete: no implicit CRT or "
          "historical NDIS imports inferred")


def probe(clang: pathlib.Path) -> None:
    if not clang.is_file():
        raise COFFError("explicit Clang executable is missing")
    with tempfile.TemporaryDirectory(prefix="whp-ndis31-llvm-") as temp:
        root = pathlib.Path(temp)
        c = root / "ndis31-abi.c"
        obj = root / "ndis31-abi.obj"
        c.write_text(FIXTURE, encoding="ascii")
        try:
            proc = subprocess.run(
                [str(clang), "--target=i386-pc-windows-msvc",
                 "-std=c11", "-ffreestanding", "-fno-builtin",
                 "-fno-stack-protector", "-Wall", "-Wextra", "-Werror",
                 "-c", str(c), "-o", str(obj)],
                capture_output=True, text=True, timeout=30, check=False)
        except (OSError, subprocess.TimeoutExpired) as exc:
            raise COFFError(f"Clang invocation failed: {exc}") from exc
        if proc.returncode:
            raise COFFError("Clang failed synthetic i386 ABI fixture:\n" +
                            proc.stderr[-1600:])
        defined, undefined = read_coff(obj)
        check_forbidden(undefined)
        if not EXPECTED_DEFINED <= defined or undefined != EXPECTED_UNDEFINED:
            raise COFFError("unexpected i386 cdecl/stdcall/NDIS import symbols: "
                            f"defined={sorted(defined)}, "
                            f"undefined={sorted(undefined)}")
        print("LLVM i386 MSVC COFF: cdecl _name, stdcall _name@bytes, "
              "direct and dllimport NDIS-symbol variants verified")
        print("Synthetic linkage only: original NT 3.51 DDK symbols still unverified")


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("mode", choices=("probe", "core"))
    p.add_argument("--clang", type=pathlib.Path)
    p.add_argument("--obj", type=pathlib.Path, action="append", default=[])
    args = p.parse_args(argv)
    try:
        if args.mode == "probe":
            if not args.clang or args.obj:
                raise COFFError("probe requires --clang and no --obj")
            probe(args.clang)
        else:
            if args.clang:
                raise COFFError("core uses --obj only")
            audit_core(args.obj)
    except (COFFError, OSError) as exc:
        print(f"WHP-Drivers: NDIS 3.1 LLVM/COFF audit rejected: {exc}",
              file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
