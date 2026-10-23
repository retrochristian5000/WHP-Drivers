"""Regression tests for NDIS 3.1 DDK fingerprint/compatibility preflight.

The fixtures are synthetic strings and are NOT reproduced historical DDKs.
"""
import contextlib
import io
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from audit_ndis31_ddk import DDKAuditError, inspect, main, sha256, verify  # noqa: E402


class DDKAuditTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.dir = Path(self.tmp.name)
        self.header = b"typedef struct _NDIS_MINIPORT_CHARACTERISTICS { int placeholder; } NDIS_MINIPORT_CHARACTERISTICS;\nvoid NdisMRegisterMiniport(void);\n"
        self.ntddk = b"/* synthetic test-only fixture */\ntypedef unsigned long ULONG;\n"
        self.write()

    def write(self):
        (self.dir / "ndis.h").write_bytes(self.header)
        (self.dir / "ntddk.h").write_bytes(self.ntddk)

    def pins(self):
        return {"ndis.h": sha256(self.header), "ntddk.h": sha256(self.ntddk)}

    def test_valid_byte_pins_are_reproducible_not_authentication(self):
        with contextlib.redirect_stdout(io.StringIO()):
            verify(self.dir, self.pins())

    def test_inspect_is_read_only(self):
        before = self.pins()
        with contextlib.redirect_stdout(io.StringIO()) as output:
            hashes, later = inspect(self.dir)
        self.assertEqual(hashes, before)
        self.assertEqual(later, ())
        self.assertIn("NOT authentication", output.getvalue())
        self.assertEqual(sha256((self.dir / "ndis.h").read_bytes()), before["ndis.h"])

    def test_mismatched_hash_fails_closed(self):
        pins = self.pins()
        pins["ndis.h"] = "0" * 64
        with contextlib.redirect_stdout(io.StringIO()):
            with self.assertRaises(DDKAuditError):
                verify(self.dir, pins)

    def test_malformed_hash_fails_closed(self):
        pins = self.pins()
        pins["ndis.h"] = "not-a-sha256"
        with self.assertRaises(DDKAuditError):
            verify(self.dir, pins)

    def test_modern_api_even_if_hash_pinned_fails(self):
        self.header += b"\ntypedef struct _NDIS_MINIPORT_DRIVER_CHARACTERISTICS {int a;} bogus;\n"
        self.write()
        with contextlib.redirect_stdout(io.StringIO()):
            with self.assertRaises(DDKAuditError):
                verify(self.dir, self.pins())

    def test_ndis4_only_send_packets_fails(self):
        self.header += b"\nvoid MiniportSendPackets(void);\n"
        self.write()
        with contextlib.redirect_stdout(io.StringIO()):
            with self.assertRaises(DDKAuditError):
                verify(self.dir, self.pins())

    def test_comments_do_not_trigger_modern_api_guard(self):
        self.header += b"\n/* NdisMRegisterMiniportDriver NET_BUFFER_LIST */\n"
        self.write()
        with contextlib.redirect_stdout(io.StringIO()):
            verify(self.dir, self.pins())

    def test_missing_headers_fail(self):
        (self.dir / "ntddk.h").unlink()
        with self.assertRaises(DDKAuditError):
            inspect(self.dir)

    def test_symlinked_headers_fail(self):
        (self.dir / "ntddk.h").unlink()
        (self.dir / "ntddk.h").symlink_to(self.dir / "ndis.h")
        with self.assertRaises(DDKAuditError):
            inspect(self.dir)

    def test_cli_verify_requires_both_pins(self):
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(main(["verify", str(self.dir), "--ndis-sha256", self.pins()["ndis.h"]]), 1)

    def test_cli_inspect_does_not_accept_pins(self):
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(main(["inspect", str(self.dir), "--ntddk-sha256", self.pins()["ntddk.h"]]), 1)


if __name__ == "__main__":
    unittest.main()
