#!/usr/bin/env python3
"""Fail-closed validation for the NOT-INSTALLABLE Win95 Radeon INF scaffold.

This checks the current fixed research-only profile, NOT arbitrary INF syntax.
Do not expand accepted install directives without verified Win95 RTM binaries.
"""
from __future__ import annotations

from pathlib import Path
import sys

SECTIONS = ("version", "manufacturer", "whp.radeonmodels", "strings")
EXPECTED = {
    "version": {
        "signature": '"$CHICAGO$"',
        "class": "Display",
        "provider": "%WHP%",
    },
    "manufacturer": {"%whp%": "WHP.RadeonModels"},
    "whp.radeonmodels": {},
    "strings": {
        "whp": '"WHP-Drivers (research only)"',
        "radeon6410d": '"AMD Radeon HD 6410D (prototype, not installable)"',
    },
}
CANDIDATE_IDS = ("PCI\\VEN_1002&DEV_9644", "PCI\\VEN_1002&DEV_9645")


class InfError(ValueError):
    """The Radeon INF is no longer a safe, unbound research skeleton."""


def validate(text: str) -> None:
    """Check exact Win95 research metadata; reject *every* added active key."""
    if "\x00" in text:
        raise InfError("binary/NUL contents are not an INF file")
    if any(ord(ch) > 127 for ch in text):
        raise InfError("research INF must remain plain ASCII")
    sections: dict[str, dict[str, str]] = {}
    comments = []
    current = None
    for lineno, raw in enumerate(text.splitlines(), 1):
        line = raw.strip()
        if not line:
            continue
        if line.startswith(";"):
            comments.append(line[1:].strip())
            continue
        if line.startswith("["):
            if not line.endswith("]"):
                raise InfError(f"line {lineno}: invalid INF section")
            current = line[1:-1].strip().lower()
            if current not in SECTIONS or current in sections:
                raise InfError(f"line {lineno}: unexpected/duplicate INF section {current}")
            sections[current] = {}
            continue
        if current is None or "=" not in line or ";" in line:
            raise InfError(f"line {lineno}: malformed/unsafe active INF entry")
        key, value = (part.strip() for part in line.split("=", 1))
        key = key.lower()
        if not key or not value or key in sections[current]:
            raise InfError(f"line {lineno}: empty/duplicate INF key {key}")
        sections[current][key] = value

    if tuple(sections) != SECTIONS:
        raise InfError("required Win95 INF sections missing or reordered")
    if sections != EXPECTED:
        raise InfError("unsafe INF change: exact metadata differs; no install/binding enabled")
    # Protect against an accidental replacement of the unverified IDs or
    # accidental loss of explicit safety notice.
    comment_text = "\n".join(comments)
    if "DELIBERATELY NON-INSTALLABLE" not in comment_text:
        raise InfError("missing non-installable warning")
    if not all(value in comment_text for value in CANDIDATE_IDS):
        raise InfError("unconfirmed PCI candidates must remain documented as comments")


def main(argv: list[str]) -> int:
    if len(argv) > 2:
        print("usage: verify_radeon_inf.py [path/to/R6410D.INF]", file=sys.stderr)
        return 2
    path = (
        Path(argv[1])
        if len(argv) == 2
        else Path(__file__).resolve().parents[1] / "packaging/win95rtm/R6410D.INF"
    )
    try:
        validate(path.read_text(encoding="ascii"))
    except (InfError, OSError, UnicodeError) as exc:
        print(f"WHP-Drivers: unsafe Radeon INF ({path}): {exc}", file=sys.stderr)
        return 1
    print("WHP-Drivers: Win95 Radeon INF research skeleton validated (NOT INSTALLABLE)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
