"""Negative regressions: research INF cannot silently become a NIC installer."""
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from verify_rtl8111e_inf import InfError, validate  # noqa: E402

INF = ROOT / "packaging/win95rtm/RTL8111E.INF"


class Win95RTL8111EInfTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.sample = INF.read_text(encoding="ascii")

    def reject(self, altered):
        with self.assertRaises(InfError):
            validate(altered)

    def test_inert_scaffold(self):
        validate(self.sample)

    def test_model_binding_for_family_id_rejected(self):
        self.reject(self.sample.replace(
            "; Intentionally empty",
            "%RTL8111E%=RTL8111E.ndi,PCI\\VEN_10EC&DEV_8168\n; Intentionally empty"))

    def test_model_binding_with_subsystem_rejected(self):
        self.reject(self.sample.replace(
            "; Intentionally empty",
            "%RTL8111E%=RTL8111E.ndi,PCI\\VEN_10EC&DEV_8168&SUBSYS_12345678\n; Intentionally empty"))

    def test_unknown_install_and_addreg_sections_rejected(self):
        self.reject(self.sample + "\n[RTL8111E.ndi]\nAddReg=NIC.reg\n")
        self.reject(self.sample + "\n[NIC.reg]\nHKR,,DevLoader,,*ndis\n")

    def test_nt_services_and_wdm_must_not_bleed(self):
        self.reject(self.sample + "\n[RTL8111E.ndi.NT.Services]\nAddService=RTL8111E,2,Svc\n")
        self.reject(self.sample.replace("Class=Net", "Class=Net\nClassGUID={4d36e972-e325-11ce-bfc1-08002be10318}"))

    def test_no_driver_copy_and_destination(self):
        self.reject(self.sample + "\n[RTL8111E.Files]\nRTL8111E.SYS\n")
        self.reject(self.sample + "\n[SourceDisksFiles]\nrtl8111e.sys=1\n")
        self.reject(self.sample + "\n[DestinationDirs]\nDefaultDestDir=11\n")

    def test_no_binding_or_device_loader_in_active_version(self):
        self.reject(self.sample.replace("Class=Net", "Class=Net\nDevLoader=*ndis"))
        self.reject(self.sample.replace("Class=Net", "Class=Net\nDeviceVxDs=rtl8111e.sys"))

    def test_invalid_signature_or_class(self):
        self.reject(self.sample.replace("$CHICAGO$", "$Windows NT$"))
        self.reject(self.sample.replace("Class=Net", "Class=NetService"))

    def test_duplicate_sections_and_registry_entries(self):
        self.reject(self.sample + "\n[Strings]\nNEW=\"something\"\n")
        self.reject(self.sample.replace("Provider=%WHP%", "Provider=%WHP%\nProvider=%WHP%"))

    def test_no_hidden_directives_inline_or_outside_section(self):
        self.reject(self.sample.replace("Class=Net", "Class=Net;AddReg=NIC.reg"))
        self.reject("AddReg=NIC.reg\n" + self.sample)
        self.reject(self.sample.replace("Class=Net", "Class=Net\n#not-an-INF-directive"))

    def test_no_unsupported_encoding(self):
        self.reject(self.sample + ";é\n")
        self.reject(self.sample + "\0")
        self.reject(self.sample + "X" * 33000)

    def test_required_evidence_cannot_disappear(self):
        self.reject(self.sample.replace("DELIBERATELY NON-INSTALLABLE", "WORK IN PROGRESS"))
        self.reject(self.sample.replace("PCI\\VEN_10EC&DEV_8168", "PCI\\VEN_10EC&DEV_8136"))
        self.reject(self.sample.replace("UpperRange=ndis3", "UpperRange=ndis5"))

    def test_active_metadata_locked(self):
        self.reject(self.sample.replace('"WHP-Drivers (research only)"', '"Realtek"'))
        self.reject(self.sample.replace("%WHP%=WHP.RTL8111E", "%WHP%=RTL8111E"))


if __name__ == "__main__":
    unittest.main()
