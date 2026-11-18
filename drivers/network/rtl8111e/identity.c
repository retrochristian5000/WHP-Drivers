#include "rtl8111e.h"

/* PCI configuration snapshots come from another trusted, read-only
 * inventory source. Decode bytes explicitly; never cast to a packed struct.
 */
static uint16_t pci_read_le16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t pci_read_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static enum whp_rtl8111e_pci_caps_state
pci_scan_caps(const uint8_t *cfg, size_t length,
              struct whp_rtl8111e_pci_snapshot *s)
{
    uint8_t visited[64] = {0};
    unsigned int hops;
    uint8_t at = cfg[0x34u];

    if (at == 0u)
        return WHP_PCI_CAPS_MALFORMED;

    /* Standard capability list is limited to the first 256 bytes.
     * Reject misaligned pointers, cycles and inaccessible entries.
     */
    for (hops = 0; hops < 48u; ++hops) {
        uint8_t id, next;
        if (at < 0x40u || at > 0xfcu || (at & 3u) != 0u)
            return WHP_PCI_CAPS_MALFORMED;
        if (visited[at >> 2])
            return WHP_PCI_CAPS_MALFORMED;
        if ((size_t)at + 2u > length)
            return WHP_PCI_CAPS_TRUNCATED;
        visited[at >> 2] = 1;
        id = cfg[at];
        next = cfg[at + 1u];
        if (id == 0xffu)
            return WHP_PCI_CAPS_MALFORMED;
        if (id == 0x01u) s->pm_cap_offset = at;
        if (id == 0x05u) s->msi_cap_offset = at;
        if (id == 0x10u) s->pcie_cap_offset = at;
        if (id == 0x11u) s->msix_cap_offset = at;
        if (next == 0u)
            return WHP_PCI_CAPS_COMPLETE;
        at = next;
    }
    return WHP_PCI_CAPS_MALFORMED;
}

enum whp_rtl8111e_pci_snapshot_result
whp_rtl8111e_decode_pci_snapshot(const uint8_t *cfg, size_t length,
                                 struct whp_rtl8111e_pci_snapshot *snapshot)
{
    struct whp_rtl8111e_pci_snapshot s = {0};
    unsigned int i;
    if (!cfg || !snapshot || length < WHP_RTL8111E_PCI_HEADER_BYTES)
        return WHP_PCI_SNAPSHOT_BAD_INPUT;

    s.id.vendor = pci_read_le16(cfg);
    s.id.device = pci_read_le16(cfg + 2u);
    if (s.id.vendor == 0xffffu || s.id.vendor == 0u)
        return WHP_PCI_SNAPSHOT_NO_DEVICE;
    s.header_type = (uint8_t)(cfg[0x0eu] & 0x7fu);
    if (s.header_type != 0u)
        return WHP_PCI_SNAPSHOT_NOT_ENDPOINT;

    s.multifunction = (uint8_t)((cfg[0x0eu] & 0x80u) != 0u);
    s.pci_revision = cfg[0x08u];
    s.programming_interface = cfg[0x09u];
    s.id.subclass = cfg[0x0au];
    s.id.class_code = cfg[0x0bu];
    s.command = pci_read_le16(cfg + 0x04u);
    s.status = pci_read_le16(cfg + 0x06u);
    for (i = 0; i < 6u; ++i)
        s.raw_bar[i] = pci_read_le32(cfg + 0x10u + i * 4u);
    s.subsystem_vendor = pci_read_le16(cfg + 0x2cu);
    s.subsystem_device = pci_read_le16(cfg + 0x2eu);
    s.irq_line = cfg[0x3cu];
    s.irq_pin = cfg[0x3du];
    s.family = whp_rtl8111e_identify_pci(&s.id);

    if ((s.status & 0x0010u) == 0u) {
        s.capabilities = WHP_PCI_CAPS_NOT_ADVERTISED;
    } else {
        s.capabilities = pci_scan_caps(cfg, length, &s);
        if (s.capabilities != WHP_PCI_CAPS_COMPLETE) {
            /* A partial or invalid chain proves no optional capability. */
            s.pcie_cap_offset = 0;
            s.pm_cap_offset = 0;
            s.msi_cap_offset = 0;
            s.msix_cap_offset = 0;
        }
    }
    *snapshot = s;
    return WHP_PCI_SNAPSHOT_OK;
}


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
