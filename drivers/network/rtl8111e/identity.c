#include "rtl8111e.h"

enum whp_rtl8111e_identity
whp_rtl8111e_identify_pci(const struct whp_rtl8111e_pci_id *id)
{
    if (!id || id->vendor != WHP_RTL8111E_PCI_VENDOR ||
        id->device != WHP_RTL8111E_PCI_DEVICE ||
        id->class_code != WHP_RTL8111E_PCI_CLASS ||
        id->subclass != WHP_RTL8111E_PCI_SUBCLASS)
        return WHP_RTL8111E_NO_MATCH;

    return WHP_RTL8111E_FAMILY_CANDIDATE;
}

enum whp_rtl8111e_identity
whp_rtl8111e_identify_txconfig(const struct whp_rtl8111e_pci_id *id,
                               uint32_t txconfig)
{
    uint32_t xid;

    if (whp_rtl8111e_identify_pci(id) != WHP_RTL8111E_FAMILY_CANDIDATE)
        return WHP_RTL8111E_NO_MATCH;

    /* Linux r8169 extracts (TxConfig >> 20) & 0xfcf before revision tests.
     * Match E1 before E2, since the E2 mask also matches E1's bits.
     */
    xid = (txconfig >> 20) & 0xfcfu;
    if ((xid & 0x7cfu) == 0x2c1u)
        return WHP_RTL8111E_E1_CANDIDATE;
    if ((xid & 0x7c8u) == 0x2c0u)
        return WHP_RTL8111E_E2_CANDIDATE;
    if ((xid & 0x7c8u) == 0x2c8u)
        return WHP_RTL8111E_EVL_CANDIDATE;

    return WHP_RTL8111E_FAMILY_CANDIDATE;
}

int whp_rtl8111e_mac_valid(const uint8_t mac[6])
{
    unsigned int i;
    int nonzero = 0;

    if (!mac || (mac[0] & 1u))
        return 0;
    for (i = 0; i < 6; ++i)
        if (mac[i])
            nonzero = 1;
    return nonzero;
}
