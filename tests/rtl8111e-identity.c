/* Pure host test: no PCI / MMIO access or Windows driver installation. */
#include "rtl8111e.h"

#define CHECK(expr) do { if (!(expr)) return __LINE__; } while (0)

int main(void)
{
    struct whp_rtl8111e_pci_id pci = {
        WHP_RTL8111E_PCI_VENDOR, WHP_RTL8111E_PCI_DEVICE,
        WHP_RTL8111E_PCI_CLASS, WHP_RTL8111E_PCI_SUBCLASS
    };
    const uint8_t real_mac[6] = { 0x02u, 0x11u, 0x22u, 0x33u, 0x44u, 0x55u };
    const uint8_t zero_mac[6] = { 0, 0, 0, 0, 0, 0 };
    const uint8_t multicast_mac[6] = { 0x01u, 0, 0, 0, 0, 1 };
    const uint8_t broadcast_mac[6] = { 255, 255, 255, 255, 255, 255 };

    CHECK(whp_rtl8111e_identify_pci(&pci) == WHP_RTL8111E_FAMILY_CANDIDATE);
    CHECK(whp_rtl8111e_identify_txconfig(&pci, 0x2c100000u) == WHP_RTL8111E_E1_CANDIDATE);
    CHECK(whp_rtl8111e_identify_txconfig(&pci, 0x2c000000u) == WHP_RTL8111E_E2_CANDIDATE);
    CHECK(whp_rtl8111e_identify_txconfig(&pci, 0x2c200000u) == WHP_RTL8111E_E2_CANDIDATE);
    CHECK(whp_rtl8111e_identify_txconfig(&pci, 0x2c800000u) == WHP_RTL8111E_EVL_CANDIDATE);
    CHECK(whp_rtl8111e_identify_txconfig(&pci, 0x48000000u) == WHP_RTL8111E_FAMILY_CANDIDATE);
    CHECK(whp_rtl8111e_identify_txconfig(&pci, 0xffffffffu) == WHP_RTL8111E_FAMILY_CANDIDATE);
    CHECK(whp_rtl8111e_identify_pci(0) == WHP_RTL8111E_NO_MATCH);
    CHECK(whp_rtl8111e_identify_txconfig(0, 0x2c100000u) == WHP_RTL8111E_NO_MATCH);

    pci.device = 0x8139u;
    CHECK(whp_rtl8111e_identify_pci(&pci) == WHP_RTL8111E_NO_MATCH);
    CHECK(whp_rtl8111e_identify_txconfig(&pci, 0x2c100000u) == WHP_RTL8111E_NO_MATCH);
    pci.device = WHP_RTL8111E_PCI_DEVICE;
    pci.vendor = 0x1022u;
    CHECK(whp_rtl8111e_identify_pci(&pci) == WHP_RTL8111E_NO_MATCH);
    pci.vendor = WHP_RTL8111E_PCI_VENDOR;
    pci.class_code = 0x03u;
    CHECK(whp_rtl8111e_identify_pci(&pci) == WHP_RTL8111E_NO_MATCH);
    pci.class_code = WHP_RTL8111E_PCI_CLASS;
    pci.subclass = 0x80u;
    CHECK(whp_rtl8111e_identify_pci(&pci) == WHP_RTL8111E_NO_MATCH);

    CHECK(whp_rtl8111e_mac_valid(real_mac));
    CHECK(!whp_rtl8111e_mac_valid(zero_mac));
    CHECK(!whp_rtl8111e_mac_valid(multicast_mac));
    CHECK(!whp_rtl8111e_mac_valid(broadcast_mac));
    CHECK(!whp_rtl8111e_mac_valid(0));
    return 0;
}
