"""Synthetic NDIS 3.x fixture: verifies our adapter code, NOT DDK authenticity."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
ADAPTER = ROOT / "drivers/network/rtl8111e/ndis31_adapter.c"
HEADER = r'''
#ifndef MOCK_NDIS31_H
#define MOCK_NDIS31_H
#include <stdint.h>
typedef uint32_t ULONG, UINT, NDIS_OID, NDIS_STATUS;
typedef uint8_t BOOLEAN;
typedef void *PVOID, *NDIS_HANDLE;
typedef NDIS_STATUS *PNDIS_STATUS;
typedef UINT *PUINT;
typedef ULONG *PULONG;
typedef BOOLEAN *PBOOLEAN;
typedef void *PNDIS_PACKET;
typedef UINT NDIS_MEDIUM, *PNDIS_MEDIUM;
#define VOID void
#define NTAPI
#define FALSE 0
#define NDIS_STATUS_SUCCESS 0
#define NDIS_STATUS_FAILURE 0xc0000001u
#define NdisMInitializeWrapper(H,A,B,C) NdisInitializeWrapper(H,A,B,C)
typedef struct {
  uint8_t MajorNdisVersion, MinorNdisVersion;
  uint32_t Reserved;
  BOOLEAN (NTAPI *CheckForHangHandler)(NDIS_HANDLE);
  void (NTAPI *DisableInterruptHandler)(NDIS_HANDLE);
  void (NTAPI *EnableInterruptHandler)(NDIS_HANDLE);
  void (NTAPI *HaltHandler)(NDIS_HANDLE);
  void (NTAPI *HandleInterruptHandler)(NDIS_HANDLE);
  NDIS_STATUS (NTAPI *InitializeHandler)(PNDIS_STATUS,PUINT,PNDIS_MEDIUM,UINT,NDIS_HANDLE,NDIS_HANDLE);
  void (NTAPI *ISRHandler)(PBOOLEAN,PBOOLEAN,NDIS_HANDLE);
  NDIS_STATUS (NTAPI *QueryInformationHandler)(NDIS_HANDLE,NDIS_OID,PVOID,ULONG,PULONG,PULONG);
  NDIS_STATUS (NTAPI *ReconfigureHandler)(PNDIS_STATUS,NDIS_HANDLE,NDIS_HANDLE);
  NDIS_STATUS (NTAPI *ResetHandler)(PBOOLEAN,NDIS_HANDLE);
  NDIS_STATUS (NTAPI *SendHandler)(NDIS_HANDLE,PNDIS_PACKET,UINT);
  NDIS_STATUS (NTAPI *SetInformationHandler)(NDIS_HANDLE,NDIS_OID,PVOID,ULONG,PULONG,PULONG);
  NDIS_STATUS (NTAPI *TransferDataHandler)(PNDIS_PACKET,PUINT,NDIS_HANDLE,NDIS_HANDLE,UINT,UINT);
} NDIS_MINIPORT_CHARACTERISTICS;
void NdisInitializeWrapper(NDIS_HANDLE *, PVOID, PVOID, PVOID);
NDIS_STATUS NdisMRegisterMiniport(NDIS_HANDLE, NDIS_MINIPORT_CHARACTERISTICS *, UINT);
void NdisTerminateWrapper(NDIS_HANDLE, PVOID);
#endif
'''
HARNESS = r'''
#include <ddk/win95rtm/network/ndis31.h>
#include <stdio.h>
#include <stdint.h>
extern NDIS_STATUS whp_rtl8111e_ndis31_registration_probe(PVOID, PVOID);
static int called, terminated, allow_handle = 1, register_ok = 1;
void NdisInitializeWrapper(NDIS_HANDLE *handle, PVOID a, PVOID b, PVOID c) {
    (void)a;(void)b;(void)c; ++called;
    *handle = allow_handle ? (NDIS_HANDLE)(uintptr_t)1 : (NDIS_HANDLE)0;
}
NDIS_STATUS NdisMRegisterMiniport(NDIS_HANDLE h, NDIS_MINIPORT_CHARACTERISTICS *chars, UINT length) {
    UINT i=42;NDIS_STATUS err=0;BOOLEAN address=1;ULONG used=7,needed=7;
    if (!h || length!=sizeof(*chars) || chars->MajorNdisVersion!=3 || chars->MinorNdisVersion!=0) return NDIS_STATUS_FAILURE;
    if (chars->ISRHandler || chars->HandleInterruptHandler || chars->CheckForHangHandler) return NDIS_STATUS_FAILURE;
    if (!chars->InitializeHandler || !chars->HaltHandler || !chars->ResetHandler || !chars->SendHandler || !chars->TransferDataHandler || !chars->QueryInformationHandler || !chars->SetInformationHandler) return NDIS_STATUS_FAILURE;
    if (chars->InitializeHandler(&err, &i, 0, 0, 0, 0)!=NDIS_STATUS_FAILURE || err!=NDIS_STATUS_FAILURE) return NDIS_STATUS_FAILURE;
    if (chars->ResetHandler(&address, 0)!=NDIS_STATUS_FAILURE || address!=0) return NDIS_STATUS_FAILURE;
    if (chars->SendHandler(0,0,0)!=NDIS_STATUS_FAILURE) return NDIS_STATUS_FAILURE;
    if (chars->TransferDataHandler(0,&i,0,0,0,0)!=NDIS_STATUS_FAILURE || i!=0) return NDIS_STATUS_FAILURE;
    if (chars->QueryInformationHandler(0,0,0,0,&used,&needed)!=NDIS_STATUS_FAILURE || used || needed) return NDIS_STATUS_FAILURE;
    used=7;needed=7;
    if (chars->SetInformationHandler(0,0,0,0,&used,&needed)!=NDIS_STATUS_FAILURE || used || needed) return NDIS_STATUS_FAILURE;
    chars->HaltHandler(0);
    return register_ok ? NDIS_STATUS_SUCCESS : NDIS_STATUS_FAILURE;
}
void NdisTerminateWrapper(NDIS_HANDLE h, PVOID specific){(void)h;(void)specific; ++terminated;}
int main(void) {
    if (whp_rtl8111e_ndis31_registration_probe(0,0)!=NDIS_STATUS_SUCCESS || terminated || called!=1) return 1;
    register_ok=0;
    if (whp_rtl8111e_ndis31_registration_probe(0,0)!=NDIS_STATUS_FAILURE || terminated!=1 || called!=2) return 2;
    allow_handle=0;
    if (whp_rtl8111e_ndis31_registration_probe(0,0)!=NDIS_STATUS_FAILURE || terminated!=1 || called!=3) return 3;
    puts("Synthetic NDIS 3.x adapter contract passed");return 0;
}
'''


class AdapterTests(unittest.TestCase):
    def setUp(self):
        self.cc = shutil.which("clang")
        if not self.cc:
            self.skipTest("clang unavailable")
        self.dir = tempfile.TemporaryDirectory()
        self.addCleanup(self.dir.cleanup)
        path = Path(self.dir.name) / "ddk/win95rtm/network"
        path.mkdir(parents=True)
        (path / "ndis31.h").write_text(HEADER, encoding="utf-8")
        self.harness = Path(self.dir.name) / "harness.c"
        self.harness.write_text(HARNESS, encoding="utf-8")

    def compile(self, *args):
        return subprocess.run(
            [self.cc, "-std=c11", "-Wall", "-Wextra", "-Werror",
             "-I" + self.dir.name, *args],
            capture_output=True, text=True, timeout=20)

    def test_callback_signatures_and_failure_paths(self):
        target = str(Path(self.dir.name) / "test")
        result = self.compile("-DWHP_RTL8111E_NDIS31_PROBE_ONLY",
                              "-DWHP_WIN95_NDIS31_USE_HISTORICAL_DDK",
                              str(ADAPTER), str(self.harness), "-o", target)
        self.assertEqual(result.returncode, 0, result.stderr)
        run = subprocess.run([target], capture_output=True, text=True, timeout=10)
        self.assertEqual(run.returncode, 0, run.stderr)
        self.assertIn("passed", run.stdout)

    def test_synthetic_i386_coff(self):
        output = Path(self.dir.name) / "rtl8111e-ndis31.obj"
        result = self.compile("--target=i386-pc-windows-msvc",
                              "-ffreestanding", "-fno-builtin",
                              "-DWHP_RTL8111E_NDIS31_PROBE_ONLY",
                              "-DWHP_WIN95_NDIS31_USE_HISTORICAL_DDK",
                              "-c", str(ADAPTER), "-o", str(output))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(output.read_bytes()[:2], b"\x4c\x01")

    def test_unverified_build_rejected(self):
        result = self.compile("-fsyntax-only", str(ADAPTER))
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("compile-only research probe", result.stderr)

    def test_missing_ddk_gate_rejected(self):
        result = self.compile("-DWHP_RTL8111E_NDIS31_PROBE_ONLY",
                              "-fsyntax-only", str(ADAPTER))
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("verified NT 3.51 DDK", result.stderr)


if __name__ == "__main__":
    unittest.main()
