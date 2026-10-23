#!/usr/bin/env python3
"""Fail-closed Windows 95 RTL8111E INF research scaffold validator.

No installation or hardware-bound packaging is authorized by this module.
This is not a general Windows INF parser or a Windows 95 installer emulator.
"""
from __future__ import annotations

from pathlib import Path
import sys

PATH = Path(__file__).resolve().parents[1] / "packaging/win95rtm/RTL8111E.INF"
SECTIONS = ("version", "manufacturer", "whp.rtl8111e", "strings")
ACTIVE = {
    "version": {
        "signature": '"$CHICAGO$"',
        "class": "Net",
        "provider": "%WHP%",
    },
    "manufacturer": {"%whp%": "WHP.RTL8111E"},
    "whp.rtl8111e": {},
    "strings": {
        "whp": '"WHP-Drivers (research only)"',
        "rtl8111e": '"Realtek RTL8111E Ethernet (research, not installable)"',
    },
}
COMMENTS_REQUIRED = (
    "DELIBERATELY NON-INSTALLABLE",
    "PCI\\VEN_10EC&DEV_8168",
    "UpperRange=ndis3",
    "LowerRange=ethernet",
    "DeviceVxDs",
    "DevLoader",
    "SourceDisks",
    "Win95 RTM",
)


class InfError(ValueError):
    """An INF change may install or bind unsupported hardware."""


def validate(text: str) -> None:
    if len(text) > 32_768 or "\x00" in text or any(ord(c) > 127 for c in text):
        raise InfError("INF must be small, NUL-free, plain ASCII")
    sections: dict[str, dict[str, str]] = {}
    comments: list[str] = []
    current: str | None = None
    for line_number, raw_line in enumerate(text.splitlines(), 1):
        line = raw_line.strip()
        if not line:
            continue
        if line.startswith(";"):
            comments.append(line[1:].strip())
            continue
        if line.startswith("["):
            if not line.endswith("]") or line.count("[") != 1 or line.count("]") != 1:
                raise InfError(f"line {line_number}: malformed section declaration")
            current = line[1:-1].strip().lower()
            if current not in SECTIONS or current in sections:
                raise InfError(f"line {line_number}: unexpected or duplicate section {current}")
            sections[current] = {}
            continue
        if current is None or "=" not in line or ";" in line or line.startswith("#"):
            raise InfError(f"line {line_number}: unexpected active installation directive")
        key, value = (part.strip() for part in line.split("=", 1))
        key = key.lower()
        if not key or not value or key in sections[current]:
            raise InfError(f"line {line_number}: missing or duplicate INF entry")
        sections[current][key] = value
    if tuple(sections) != SECTIONS or sections != ACTIVE:
        raise InfError("unsafe network INF change: model/installation metadata differs")
    joined = "\n".join(comments)
    if not all(value in joined for value in COMMENTS_REQUIRED):
        raise InfError("missing non-installable warning or historical research checklist")


def main(argv: list[str]) -> int:
    if len(argv) > 2:
        print("usage: verify_rtl8111e_inf.py [path/to/RTL8111E.INF]", file=sys.stderr)
        return 2
    file = Path(argv[1]) if len(argv) == 2 else PATH
    try:
        validate(file.read_text(encoding="ascii"))
    except (InfError, OSError, UnicodeError) as exc:
        print(f"WHP-Drivers: unsafe RTL8111E INF ({file}): {exc}", file=sys.stderr)
        return 1
    print("WHP-Drivers: Win95 RTL8111E INF research scaffold validated (NOT INSTALLABLE)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
