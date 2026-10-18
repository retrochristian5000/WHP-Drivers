"""Negative tests ensure WHP's Radeon INF cannot accidentally bind a GPU."""
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from verify_radeon_inf import InfError, validate  # noqa: E402

INF = ROOT / "packaging/win95rtm/R6410D.INF"


class Win95RadeonInfTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.sample = INF.read_text(encoding="ascii")

    def test_scaffold(self):
        validate(self.sample)

    def assert_rejected(self, altered):
        with self.assertRaises(InfError):
            validate(altered)

    def test_no_pci_binding_even_with_known_device_id(self):
        self.assert_rejected(
            self.sample.replace(
                "; Intentionally empty",
                "Radeon6410D=R6410D.Install,PCI\\VEN_1002&DEV_9644\n; Intentionally empty",
            )
        )

    def test_no_mobility_binding(self):
        self.assert_rejected(
            self.sample.replace(
                "; Intentionally empty",
                "Radeon6410D=R6410D.Install,PCI\\VEN_1002&DEV_9645\n; Intentionally empty",
            )
        )

    def test_no_copy_files(self):
        self.assert_rejected(self.sample + "\n[R6410D.Install]\nCopyFiles=R6410D.Files\n")

    def test_no_registry_writes_or_ddc_enabling(self):
        self.assert_rejected(self.sample + "\n[R6410D.Reg]\nHKR,DEFAULT,DDC,,1\n")
        self.assert_rejected(self.sample.replace("Class=Display", "AddReg=R6410D.Reg"))

    def test_reject_post_95_entries(self):
        self.assert_rejected(
            self.sample.replace("Class=Display", "Class=Display\nClassGuid={4d36e968-e325-11ce-bfc1-08002be10318}")
        )
        self.assert_rejected(self.sample.replace("Class=Display", "Class=Display\nCatalogFile=radeon.cat"))

    def test_reject_nt_signature(self):
        self.assert_rejected(self.sample.replace("$CHICAGO$", "$Windows NT$"))

    def test_duplicate_sections_and_keys(self):
        self.assert_rejected(self.sample + "\n[Strings]\nExtra=\"hidden\"\n")
        self.assert_rejected(self.sample.replace("Class=Display", "Class=Display\nClass=Display"))

    def test_missing_mandatory_sections(self):
        self.assert_rejected(self.sample.replace("[Manufacturer]", "; [Manufacturer]"))

    def test_comment_only_ids_cannot_be_removed(self):
        self.assert_rejected(self.sample.replace("PCI\\VEN_1002&DEV_9644", "PCI\\VEN_1002&DEV_9640"))

    def test_reject_non_ascii_and_malformed_grammar(self):
        self.assert_rejected(self.sample + "\n;é\n")
        self.assert_rejected(self.sample.replace("Provider=%WHP%", "Provider %WHP%"))

    def test_no_incorrect_8bit_settings(self):
        self.assert_rejected(self.sample.replace("Class=Display", 'Class=Display\nMode="8,640,480"'))


if __name__ == "__main__":
    unittest.main()
