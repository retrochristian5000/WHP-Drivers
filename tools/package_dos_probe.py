#!/usr/bin/env python3
"""Package one verified 16-bit WHPPROBE MZ executable as TGZ and ZIP."""
from __future__ import annotations

import argparse
import gzip
import hashlib
import io
from pathlib import Path
import re
import tarfile
import zipfile

from verify_dos_mz import verify_mz


def package(exe: Path, output_dir: Path, source_sha: str, watcom_sha: str) -> tuple[Path, Path]:
    for label, value in (("source SHA", source_sha), ("OpenWatcom SHA", watcom_sha)):
        if re.fullmatch(r"[0-9a-f]{40}", value) is None:
            raise ValueError(f"invalid {label}: expected a full lowercase Git SHA")
    if exe.is_symlink() or not exe.is_file():
        raise ValueError("DOS executable must be a regular file, not a symlink")
    binary = exe.read_bytes()
    verify_mz(binary)
    if output_dir.is_symlink():
        raise ValueError("package output directory must not be a symlink")
    output_dir.mkdir(parents=True, exist_ok=True)

    readme = (
        "WHP-Drivers DOS hardware probe\r\n"
        "==============================\r\n"
        "Run WHPPROBE.EXE in a compatible real-mode DOS environment.\r\n"
        "Commands: WHPPROBE /?  /R  /A  /M  /S\r\n"
        "This is a diagnostic utility, not an installable hardware driver.\r\n"
        "CI checks compilation and DOS MZ structure only.\r\n"
        "DOS execution and physical hardware compatibility are NOT tested.\r\n"
    ).encode("ascii")
    manifest = (
        "WHP-Drivers DOS probe build\r\n"
        f"Source commit: {source_sha}\r\n"
        f"Pinned OpenWatcom commit: {watcom_sha}\r\n"
        f"WHPPROBE.EXE SHA256: {hashlib.sha256(binary).hexdigest()}\r\n"
        "Target: 16-bit real-mode DOS MZ EXE (OpenWatcom)\r\n"
        "Validation: MZ structural preflight; DOS runtime NOT tested\r\n"
    ).encode("ascii")
    members = {
        "WHPPROBE/WHPPROBE.EXE": binary,
        "WHPPROBE/README.TXT": readme,
        "WHPPROBE/BUILD.TXT": manifest,
    }
    stem = f"WHPPROBE-{source_sha[:12]}"
    tgz = output_dir / f"{stem}.tgz"
    zip_path = output_dir / f"{stem}.zip"

    with tgz.open("wb") as raw:
        with gzip.GzipFile(filename="", mode="wb", fileobj=raw, mtime=0) as compressed:
            with tarfile.open(mode="w", fileobj=compressed, format=tarfile.USTAR_FORMAT) as archive:
                for name, data in members.items():
                    entry = tarfile.TarInfo(name)
                    entry.size = len(data)
                    entry.mtime = 0
                    entry.mode = 0o644
                    entry.uid = entry.gid = 0
                    entry.uname = entry.gname = ""
                    archive.addfile(entry, io.BytesIO(data))

    with zipfile.ZipFile(zip_path, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        for name, data in members.items():
            entry = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
            entry.create_system = 3
            entry.external_attr = 0o100644 << 16
            entry.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(entry, data)

    checksums = "".join(
        f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.name}\n"
        for path in (tgz, zip_path)
    )
    (output_dir / "SHA256SUMS").write_text(checksums, encoding="ascii")
    return tgz, zip_path


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--source-sha", required=True)
    parser.add_argument("--watcom-sha", required=True)
    args = parser.parse_args()
    try:
        tgz, zip_path = package(args.exe, args.output_dir, args.source_sha, args.watcom_sha)
    except (OSError, ValueError, tarfile.TarError, zipfile.BadZipFile) as exc:
        parser.error(str(exc))
    print(f"WHP-Drivers: packaged {tgz.name} and {zip_path.name} (DOS runtime NOT tested)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
