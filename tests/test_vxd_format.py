"""Tests for structural VxD checking; fixtures are NOT bootable drivers."""
import struct
import unittest

from tools.verify_vxd import check_image, FormatError


def sample():
    # A deliberately synthetic LE image exercising table bounds and formats.
    image = bytearray(0x2000)
    image[:2] = b"MZ"
    struct.pack_into("<I", image, 0x3C, 0x80)
    h = 0x80
    image[h:h + 2] = b"LE"
    struct.pack_into("<H", image, h + 8, 2)  # 80386
    struct.pack_into("<H", image, h + 10, 4)  # Windows 386
    struct.pack_into("<I", image, h + 0x10, 0x38000)
    struct.pack_into("<I", image, h + 0x14, 1)
    struct.pack_into("<I", image, h + 0x28, 4096)
    struct.pack_into("<I", image, h + 0x2C, 4096)
    struct.pack_into("<I", image, h + 0x40, 0xD0)
    struct.pack_into("<I", image, h + 0x44, 1)
    struct.pack_into("<I", image, h + 0x48, 0xE8)
    struct.pack_into("<I", image, h + 0x58, 0xEC)
    struct.pack_into("<I", image, h + 0x5C, 0xF8)
    struct.pack_into("<I", image, h + 0x68, 0x110)
    struct.pack_into("<I", image, h + 0x6C, 0x118)
    struct.pack_into("<I", image, h + 0x80, 0x1000)
    struct.pack_into("<IIIII", image, h + 0xD0,
                     4096, 0, 0x2045, 1, 1)
    image[h + 0xE8:h + 0xEC] = b"\x00\x00\x01\x00"
    image[h + 0xEC:h + 0xF6] = b"\x07WHPTEST\x00\x00"
    image[h + 0xF8:h + 0x101] = b"\x01\x03\x01\x00\x00\x00\x00\x00\x00"
    return image


class FormatTests(unittest.TestCase):
    def test_synthetic_le_tables(self):
        summary = check_image(sample(), dynamic=True)
        self.assertEqual(summary["name"], "WHPTEST")
        self.assertEqual(summary["ddb_object"], 1)

    def test_no_mz(self):
        bad = sample()
        bad[0:2] = b"XX"
        with self.assertRaises(FormatError):
            check_image(bad)

    def test_reject_pe(self):
        bad = sample()
        bad[0x80:0x82] = b"PE"
        with self.assertRaises(FormatError):
            check_image(bad)

    def test_reject_bad_ord1(self):
        bad = sample()
        bad[0x80 + 0xF8 + 1] = 2
        with self.assertRaises(FormatError):
            check_image(bad)

    def test_reject_broken_page(self):
        bad = sample()
        bad[0x80 + 0xE8 + 2] = 2
        with self.assertRaises(FormatError):
            check_image(bad)

    def test_reject_truncation(self):
        with self.assertRaises(FormatError):
            check_image(sample()[:0x400])

    def test_reject_missing_dynamic_flags(self):
        bad = sample()
        struct.pack_into("<I", bad, 0x80 + 0x10, 0)
        with self.assertRaises(FormatError):
            check_image(bad, dynamic=True)


if __name__ == "__main__":
    unittest.main()
