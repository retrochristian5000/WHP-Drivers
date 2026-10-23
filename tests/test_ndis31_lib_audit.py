"""Synthetic link-input tests. These are not historical DDK libraries."""
from contextlib import redirect_stdout
import hashlib
import io
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
from types import SimpleNamespace

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from audit_ndis31_libs import (AuditError, inventory, library, object_file,
                               pins_from, resolve, parse_nm, main)


class LinkInputTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        root = Path(self.temp.name)
        self.ndis = root / "ndis.lib"
        self.ndis.write_bytes(b"!<arch>\n" + b"fake; no valid COFF members")
        self.obj = root / "rtl8111e-adapter.obj"
        self.obj.write_bytes(b"\x4c\x01" + bytes(18))
        self.nm = root / "llvm-nm"
        self.nm.write_bytes(b"synthetic tool stub")
        self.pin = "ndis.lib=" + hashlib.sha256(self.ndis.read_bytes()).hexdigest()

    def fake_run(self, cmd, **kwargs):
        path = Path(cmd[-1]).name.lower()
        if path.endswith(".obj"):
            output = "_NdisMRegisterMiniport@16 U\n_RTL_MP_DRV T 0\n"
        else:
            output = "_NdisMRegisterMiniport@16 T 0\n__imp__NdisMRegisterMiniport@16 I 0\n"
        return SimpleNamespace(returncode=0, stdout=output, stderr="")

    def test_inventory_hash_is_repeatable(self):
        with redirect_stdout(io.StringIO()) as out:
            inventory([self.ndis])
        self.assertIn(self.pin.split("=", 1)[1], out.getvalue())

    def test_reject_non_archive(self):
        self.ndis.write_bytes(b"MZ" + bytes(20))
        with self.assertRaises(AuditError):
            library(self.ndis)

    def test_reject_bad_object_architecture(self):
        self.obj.write_bytes(b"\x64\x86" + bytes(18))
        with self.assertRaises(AuditError):
            object_file(self.obj)

    def test_reject_path_alias_pin(self):
        with self.assertRaises(AuditError):
            pins_from(["../ndis.lib=" + "a"*64])

    def test_reject_duplicate_pin(self):
        with self.assertRaises(AuditError):
            pins_from([self.pin, self.pin])

    def test_reject_missing_ndis(self):
        other = self.ndis.with_name("ntoskrnl.lib")
        other.write_bytes(self.ndis.read_bytes())
        with self.assertRaises(AuditError):
            resolve([other], [self.obj], ["ntoskrnl.lib=" + hashlib.sha256(other.read_bytes()).hexdigest()], self.nm)

    def test_reject_unpinned(self):
        with self.assertRaises(AuditError):
            resolve([self.ndis], [self.obj], [], self.nm)

    def test_reject_sha_mismatch(self):
        with self.assertRaises(AuditError):
            resolve([self.ndis], [self.obj], ["ndis.lib=" + "0"*64], self.nm)

    def test_symbols_exact_decoration(self):
        undefined, defined = parse_nm("file(member):\n_NdisMRegisterMiniport@16 U\n_NdisMRegisterMiniport@20 T 0\n")
        self.assertIn("_NdisMRegisterMiniport@16", undefined)
        self.assertNotIn("_NdisMRegisterMiniport@16", defined)

    def test_link_input_preflight_all_symbols_present(self):
        with patch("audit_ndis31_libs.subprocess.run", side_effect=self.fake_run):
            with redirect_stdout(io.StringIO()) as out:
                resolve([self.ndis], [self.obj], [self.pin], self.nm)
        self.assertIn("Proposed linker INPUT ORDER", out.getvalue())

    def test_missing_symbols_rejected(self):
        def bad_run(cmd, **kwargs):
            if cmd[-1].endswith(".lib"):
                return SimpleNamespace(returncode=0, stdout="_NdisMRegisterMiniport@20 T 0\n", stderr="")
            return self.fake_run(cmd, **kwargs)
        with patch("audit_ndis31_libs.subprocess.run", side_effect=bad_run):
            with redirect_stdout(io.StringIO()), redirect_stdout(io.StringIO()):
                with self.assertRaises(AuditError):
                    resolve([self.ndis], [self.obj], [self.pin], self.nm)

    def test_no_ndis_adapter_rejected(self):
        with patch("audit_ndis31_libs.subprocess.run", return_value=SimpleNamespace(returncode=0,stdout="_whp_core T 0\n",stderr="")):
            with redirect_stdout(io.StringIO()):
                with self.assertRaises(AuditError):
                    resolve([self.ndis], [self.obj], [self.pin], self.nm)

    def test_bad_nm_format_rejected(self):
        with self.assertRaises(AuditError):
            parse_nm("unrecognized output with spaces and no symbol type\n")

    def test_cli_refuses_to_resolve_without_nm(self):
        with patch("sys.stderr", new_callable=io.StringIO):
            self.assertEqual(main(["resolve", "--lib", str(self.ndis)]), 1)


if __name__ == "__main__":
    unittest.main()
