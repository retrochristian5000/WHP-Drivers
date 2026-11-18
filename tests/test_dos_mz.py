"""Synthetic DOS MZ layout validation; no executable files created."""
from __future__ import annotations
import importlib.util
from pathlib import Path
import struct
import unittest

SCRIPT = Path(__file__).resolve().parents[1] / "tools" / "verify_dos_mz.py"
spec = importlib.util.spec_from_file_location("verify_dos_mz", SCRIPT)
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

def fixture():
    image = bytearray(512)
    image[:2] = b"MZ"
    struct.pack_into("<4H", image, 2, 0, 1, 1, 2)
    struct.pack_into("<H", image, 0x18, 0x1c)
    return image

class TestMZ(unittest.TestCase):
    def test_good_image(self):
        mod.verify_mz(bytes(fixture()))
    def test_short(self):
        with self.assertRaises(ValueError):
            mod.verify_mz(b"MZ")
    def test_bad_magic(self):
        image = fixture()
        image[:2] = b"PE"
        with self.assertRaises(ValueError):
            mod.verify_mz(bytes(image))
    def test_zero_pages(self):
        image = fixture()
        struct.pack_into("<H", image, 4, 0)
        with self.assertRaises(ValueError):
            mod.verify_mz(bytes(image))
    def test_truncated(self):
        image = fixture()
        with self.assertRaises(ValueError):
            mod.verify_mz(bytes(image[:64]))
    def test_invalid_header(self):
        image = fixture()
        struct.pack_into("<H", image, 8, 33)
        with self.assertRaises(ValueError):
            mod.verify_mz(bytes(image))
    def test_bad_relocation_table(self):
        image = fixture()
        struct.pack_into("<H", image, 0x18, 33)
        with self.assertRaises(ValueError):
            mod.verify_mz(bytes(image))
    def test_partial_page(self):
        image = fixture()
        struct.pack_into("<H", image, 2, 60)
        with self.assertRaises(ValueError):
            mod.verify_mz(bytes(image))

if __name__ == "__main__":
    unittest.main()
