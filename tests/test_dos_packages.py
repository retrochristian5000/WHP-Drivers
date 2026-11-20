"""Verify the DOS probe's TGZ/ZIP packages without running DOS."""
from __future__ import annotations

import hashlib
import importlib.util
from pathlib import Path
import struct
import sys
import tarfile
import tempfile
import unittest
import zipfile

TOOLS = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS))
spec = importlib.util.spec_from_file_location("package_dos_probe", TOOLS / "package_dos_probe.py")
module = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(module)
SOURCE = "a" * 40
WATCOM = "b" * 40


def fixture() -> bytes:
    image = bytearray(512)
    image[:2] = b"MZ"
    struct.pack_into("<4H", image, 2, 0, 1, 1, 2)
    struct.pack_into("<H", image, 0x18, 0x1c)
    return bytes(image)


class TestDOSPackages(unittest.TestCase):
    def test_both_archives_match_and_are_repeatable(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            exe = root / "WHPPROBE.EXE"
            exe.write_bytes(fixture())
            tgz, zip_path = module.package(exe, root / "out", SOURCE, WATCOM)
            with tarfile.open(tgz, "r:gz") as archive:
                tar_files = {entry.name: archive.extractfile(entry).read() for entry in archive.getmembers()}
            with zipfile.ZipFile(zip_path) as archive:
                zip_files = {name: archive.read(name) for name in archive.namelist()}
            self.assertEqual(tar_files, zip_files)
            self.assertEqual(tar_files["WHPPROBE/WHPPROBE.EXE"], fixture())
            self.assertIn(SOURCE.encode(), tar_files["WHPPROBE/BUILD.TXT"])
            self.assertIn(WATCOM.encode(), tar_files["WHPPROBE/BUILD.TXT"])
            self.assertEqual(len(tar_files), 3)
            sums = (root / "out" / "SHA256SUMS").read_text(encoding="ascii")
            for path in (tgz, zip_path):
                self.assertIn(f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.name}", sums)
            original = (tgz.read_bytes(), zip_path.read_bytes())
            module.package(exe, root / "out", SOURCE, WATCOM)
            self.assertEqual(original, (tgz.read_bytes(), zip_path.read_bytes()))

    def test_reject_corrupt_executable(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            exe = root / "WHPPROBE.EXE"
            exe.write_bytes(b"PE" + b"\x00" * 510)
            with self.assertRaises(ValueError):
                module.package(exe, root / "out", SOURCE, WATCOM)
            self.assertFalse((root / "out").exists())

    def test_reject_symlink_and_invalid_commit(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            exe = root / "WHPPROBE.EXE"
            exe.write_bytes(fixture())
            with self.assertRaises(ValueError):
                module.package(exe, root / "out", "main", WATCOM)
            link = root / "LINK.EXE"
            link.symlink_to(exe)
            with self.assertRaises(ValueError):
                module.package(link, root / "out", SOURCE, WATCOM)


if __name__ == "__main__":
    unittest.main()
