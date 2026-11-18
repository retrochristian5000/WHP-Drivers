/* Synthetic PCI config bytes only; no live device I/O or driver loading. */
#include "rtl8111e.h"
#include <stdio.h>
#include <string.h>

#define CHECK(x) do { ++checks; if (!(x)) { \
    fprintf(stderr, "PCI snapshot failed line %d: %s\n", __LINE__, #x); \
    return 1; } } while (0)

static void put16(uint8_t *p, unsigned int off, uint16_t v)
{
    p[off] = (uint8_t)v;
    p[off + 1u] = (uint8_t)(v >> 8);
}
static void put32(uint8_t *p, unsigned int off, uint32_t v)
{
    put16(p, off, (uint16_t)v);
    put16(p, off + 2u, (uint16_t)(v >> 16));
}
static void fixture(uint8_t cfg[256])
{
    memset(cfg, 0, 256u);
    put16(cfg, 0, WHP_RTL8111E_PCI_VENDOR);
    put16(cfg, 2, WHP_RTL8111E_PCI_DEVICE);
    put16(cfg, 4, WHP_PCI_COMMAND_MEMORY_SPACE | WHP_PCI_COMMAND_BUS_MASTER);
    put16(cfg, 6, 0x0010u); /* Capability-list status bit. */
    cfg[8] = 0x07u;  /* PCI revision ID, NOT Realtek silicon revision. */
    cfg[9] = 0x01u;  /* Preserve supplied programming interface. */
    cfg[10] = WHP_RTL8111E_PCI_SUBCLASS;
    cfg[11] = WHP_RTL8111E_PCI_CLASS;
    cfg[14] = 0x80u; /* Multifunction endpoint (type 0). */
    put32(cfg, 0x10u, 0xfebc0004u); /* Raw 64-bit BAR low half. */
    put32(cfg, 0x14u, 0x00000001u); /* Raw high half. */
    put16(cfg, 0x2cu, 0x103cu);
    put16(cfg, 0x2eu, 0x2310u);
    cfg[0x34] = 0x50u;
    cfg[0x3c] = 11u;
    cfg[0x3d] = 1u;
    cfg[0x50] = 0x01u; cfg[0x51] = 0x60u; /* PM */
    cfg[0x60] = 0x05u; cfg[0x61] = 0x70u; /* MSI */
    cfg[0x70] = 0x10u; cfg[0x71] = 0u;    /* PCI Express */
}
int main(void)
{
    uint8_t cfg[256];
    struct whp_rtl8111e_pci_snapshot s;
    int checks = 0;
    fixture(cfg);
    CHECK(whp_rtl8111e_decode_pci_snapshot(0, sizeof(cfg), &s) == WHP_PCI_SNAPSHOT_BAD_INPUT);
    CHECK(whp_rtl8111e_decode_pci_snapshot(cfg, sizeof(cfg), 0) == WHP_PCI_SNAPSHOT_BAD_INPUT);
    CHECK(whp_rtl8111e_decode_pci_snapshot(cfg, 63u, &s) == WHP_PCI_SNAPSHOT_BAD_INPUT);
    CHECK(whp_rtl8111e_decode_pci_snapshot(cfg, sizeof(cfg), &s) == WHP_PCI_SNAPSHOT_OK);
    CHECK(s.family == WHP_RTL8111E_FAMILY_CANDIDATE);
    CHECK(s.id.vendor == 0x10ecu && s.id.device == 0x8168u);
    CHECK(s.pci_revision == 7u && s.programming_interface == 1u);
    CHECK(s.header_type == 0u && s.multifunction == 1u);
    CHECK(s.command == 6u && (s.command & WHP_PCI_COMMAND_BUS_MASTER) != 0);
    CHECK(s.subsystem_vendor == 0x103cu && s.subsystem_device == 0x2310u);
    CHECK(s.raw_bar[0] == 0xfebc0004u && s.raw_bar[1] == 1u);
    CHECK(s.irq_line == 11u && s.irq_pin == 1u);
    CHECK(s.capabilities == WHP_PCI_CAPS_COMPLETE);
    CHECK(s.pm_cap_offset == 0x50u && s.msi_cap_offset == 0x60u);
    CHECK(s.pcie_cap_offset == 0x70u && s.msix_cap_offset == 0u);
    CHECK(whp_rtl8111e_decode_pci_snapshot(cfg, 64u, &s) == WHP_PCI_SNAPSHOT_OK);
    CHECK(s.capabilities == WHP_PCI_CAPS_TRUNCATED && s.pcie_cap_offset == 0u);
    CHECK(whp_rtl8111e_decode_pci_snapshot(cfg, 0x52u, &s) == WHP_PCI_SNAPSHOT_OK);
    CHECK(s.capabilities == WHP_PCI_CAPS_TRUNCATED && s.pm_cap_offset == 0u);
    CHECK(whp_rtl8111e_decode_pci_snapshot(cfg, 256u, &s) == WHP_PCI_SNAPSHOT_OK);
    CHECK(s.capabilities == WHP_PCI_CAPS_COMPLETE && s.pcie_cap_offset == 0x70u);
    cfg[0x71] = 0x50u; /* Reject cyclic chains, even after PCIe entry. */
    CHECK(whp_rtl8111e_decode_pci_snapshot(cfg, 256u, &s) == WHP_PCI_SNAPSHOT_OK);
    CHECK(s.capabilities == WHP_PCI_CAPS_MALFORMED && s.pcie_cap_offset == 0u);
    fixture(cfg);
    cfg[0x34] = 0x51u; /* Misaligned pointer. */
    CHECK(whp_rtl8111e_decode_pci_snapshot(cfg, 256u, &s) == WHP_PCI_SNAPSHOT_OK);
    CHECK(s.capabilities == WHP_PCI_CAPS_MALFORMED);
    fixture(cfg);
    cfg[0x34] = 0xffu; /* Pointer beyond first 256 bytes. */
    CHECK(whp_rtl8111e_decode_pci_snapshot(cfg, 256u, &s) == WHP_PCI_SNAPSHOT_OK);
    CHECK(s.capabilities == WHP_PCI_CAPS_MALFORMED);
    fixture(cfg);
    cfg[0x34] = 0u;
    CHECK(whp_rtl8111e_decode_pci_snapshot(cfg, 256u, &s) == WHP_PCI_SNAPSHOT_OK);
    CHECK(s.capabilities == WHP_PCI_CAPS_MALFORMED);
    fixture(cfg);
    cfg[0x50] = 0xffu;
    CHECK(whp_rtl8111e_decode_pci_snapshot(cfg, 256u, &s) == WHP_PCI_SNAPSHOT_OK);
    CHECK(s.capabilities == WHP_PCI_CAPS_MALFORMED);
    fixture(cfg);
    put16(cfg, 6u, 0u);
    CHECK(whp_rtl8111e_decode_pci_snapshot(cfg, 64u, &s) == WHP_PCI_SNAPSHOT_OK);
    CHECK(s.capabilities == WHP_PCI_CAPS_NOT_ADVERTISED && s.pcie_cap_offset == 0u);
    fixture(cfg);
    put16(cfg, 2u, 0x8139u); /* Different Realtek device. */
    CHECK(whp_rtl8111e_decode_pci_snapshot(cfg, 256u, &s) == WHP_PCI_SNAPSHOT_OK);
    CHECK(s.family == WHP_RTL8111E_NO_MATCH);
    fixture(cfg);
    cfg[14] = 1u; /* Type-1 PCI bridge, not a NIC endpoint. */
    CHECK(whp_rtl8111e_decode_pci_snapshot(cfg, 256u, &s) == WHP_PCI_SNAPSHOT_NOT_ENDPOINT);
    fixture(cfg);
    put16(cfg, 0, 0xffffu); /* Absent/unreadable function. */
    CHECK(whp_rtl8111e_decode_pci_snapshot(cfg, 256u, &s) == WHP_PCI_SNAPSHOT_NO_DEVICE);
    fixture(cfg);
    put16(cfg, 0, 0u);
    CHECK(whp_rtl8111e_decode_pci_snapshot(cfg, 256u, &s) == WHP_PCI_SNAPSHOT_NO_DEVICE);
    printf("RTL8111E PCI snapshot: %d checks passed\n", checks);
    return 0;
}
