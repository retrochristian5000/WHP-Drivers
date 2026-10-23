"""NDIS 3.1 LLVM object ABI tests. Synthetic fixtures, not a historical DDK."""
from contextlib import redirect_stdout
import io
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from check_ndis31_llvm import (COFFError, audit_core, check_forbidden,
                               probe, read_coff)

class NDIS31LLVMTests(unittest.TestCase):
    def setUp(self):
        self.work = tempfile.TemporaryDirectory(prefix="whp-llvm-unit-")
        self.addCleanup(self.work.cleanup)
        self.root = Path(self.work.name)
        self.clang = shutil.which("clang")

    def object(self, name: str, body: str) -> Path:
        if not self.clang:
            self.skipTest("host Clang not found")
        src = self.root / (name + ".c")
        obj = self.root / (name + ".obj")
        src.write_text(body, encoding="ascii")
        p = subprocess.run([self.clang, "--target=i386-pc-windows-msvc",
                            "-std=c11", "-ffreestanding", "-fno-builtin",
                            "-fno-stack-protector", "-Wall", "-Wextra",
                            "-Werror", "-c", str(src), "-o", str(obj)],
                           capture_output=True, text=True, timeout=20)
        self.assertEqual(p.returncode, 0, p.stderr)
        return obj

    def test_compiler_produces_correct_decorated_ndis_import_variants(self):
        if not self.clang:
            self.skipTest("host Clang not found")
        with redirect_stdout(io.StringIO()) as out:
            probe(Path(self.clang))
        self.assertIn("cdecl _name", out.getvalue())

    def test_coff_long_names_and_exact_x86_stdcall(self):
        obj = self.object("stdcall", """
            unsigned __attribute__((stdcall)) whp_send(void *a, unsigned b)
            { return (a != 0) + b; }
            unsigned whp_cdecl(void) { return 1u; }
        """)
        defined, missing = read_coff(obj)
        self.assertEqual(missing, set())
        self.assertIn("_whp_send@8", defined)
        self.assertIn("_whp_cdecl", defined)
        self.assertNotIn("_whp_send", defined)

    def test_linker_directive_default_library_is_rejected(self):
        obj = self.object("defaultlib", """
            #pragma comment(lib, "kernel32.lib")
            unsigned whp_test(void) { return 1u; }
        """)
        with self.assertRaisesRegex(COFFError, "default library"):
            read_coff(obj)

    def test_core_group_symbol_resolution(self):
        obj1 = self.object("caller", """
            extern unsigned whp_helper(unsigned);
            unsigned whp_client(unsigned n) { return whp_helper(n); }
        """)
        obj2 = self.object("helper", """
            unsigned whp_helper(unsigned n) { return n + 1u; }
        """)
        with self.assertRaisesRegex(COFFError, "unresolved"):
            audit_core([obj1])
        with redirect_stdout(io.StringIO()):
            audit_core([obj1, obj2])

    def test_forbidden_compiler_runtime_helpers(self):
        with self.assertRaises(COFFError):
            check_forbidden({"__chkstk", "_memcpy", "__security_cookie"})
        with self.assertRaises(COFFError):
            check_forbidden({"__imp__security_check_cookie"})
        check_forbidden({"_whp_rtl8111e_mp_begin_init"})

    def test_corrupt_object_and_wrong_target_fail_closed(self):
        corrupt = self.root / "bad.obj"
        corrupt.write_bytes(b"MZ" + b"\x00" * 128)
        with self.assertRaises(COFFError):
            read_coff(corrupt)
        corrupt.write_bytes(b"\x4c\x01")
        with self.assertRaises(COFFError):
            read_coff(corrupt)
        if self.clang:
            obj = self.root / "x64.obj"
            src = self.root / "x64.c"
            src.write_text("unsigned whp(void) { return 0; }\n", encoding="ascii")
            result = subprocess.run(
                [self.clang, "--target=x86_64-pc-windows-msvc",
                 "-c", str(src), "-o", str(obj)],
                capture_output=True, text=True, timeout=20)
            self.assertEqual(result.returncode, 0, result.stderr)
            with self.assertRaisesRegex(COFFError, "i386"):
                read_coff(obj)

if __name__ == "__main__":
    unittest.main()
