/* Synthetic type-1 PCI bridge snapshots only: no physical I/O. */
#include "pci_bridge.h"
#include <stdio.h>
#include <string.h>

#define CHECK(x) do { ++checks; if (!(x)) { \
    fprintf(stderr, "PCI bridge test failed at line %d: %s\n", __LINE__, #x); \
    return 1; } } while (0)
static void put16(uint8_t *p, unsigned int off, uint16_t v)
{
    p[off] = (uint8_t)v;
    p[off + 1] = (uint8_t)(v >> 8);
}
static void put32(uint8_t *p, unsigned int off, uint32_t v)
{
    put16(p, off, (uint16_t)v);
    put16(p, off + 2, (uint16_t)(v >> 16));
}
static void make_bridge(uint8_t cfg[256])
{
    memset(cfg, 0, 256u);
    put16(cfg, 0, 0x1022u); /* Illustrative AMD vendor; not Pavilion probe. */
    put16(cfg, 2, 0x4321u); /* Synthetic ID, not an asserted A55 ID. */
    put16(cfg, 4, 0x0006u); /* Memory decode/bus master observed. */
    put16(cfg, 6, 0x0010u); /* Capability list advertised. */
    cfg[8] = 3;
    cfg[10] = 4;
    cfg[11] = 6;
    cfg[14] = 0x81; /* Multifunction, type 1. */
    cfg[0x18] = 0;
    cfg[0x19] = 3;
    cfg[0x1a] = 3;
    cfg[0x1c] = 0x20;
    cfg[0x1d] = 0x30;
    put16(cfg, 0x20, 0x8000);
    put16(cfg, 0x22, 0x8ff0);
    put16(cfg, 0x24, 0x9001); /* 64-bit pref window. */
    put16(cfg, 0x26, 0x90f1);
    put32(cfg, 0x28, 1);
    put32(cfg, 0x2c, 1);
    cfg[0x34] = 0x50;
    cfg[0x50] = 0x01; cfg[0x51] = 0x60;
    cfg[0x60] = 0x10; cfg[0x61] = 0x00;
    cfg[0x3c] = 11;
    cfg[0x3d] = 1;
}
int main(void)
{
    uint8_t cfg[256];
    struct whp_pci95_bridge b;
    int checks = 0;
    make_bridge(cfg);
    CHECK(whp_pci95_decode_bridge(0, sizeof(cfg), &b) == WHP_PCI95_PARSE_BAD_INPUT);
    CHECK(whp_pci95_decode_bridge(cfg, sizeof(cfg), 0) == WHP_PCI95_PARSE_BAD_INPUT);
    CHECK(whp_pci95_decode_bridge(cfg, 63, &b) == WHP_PCI95_PARSE_BAD_INPUT);
    CHECK(whp_pci95_decode_bridge(cfg, 256, &b) == WHP_PCI95_PARSE_OK);
    CHECK(b.vendor == 0x1022 && b.device == 0x4321);
    CHECK(b.class_code == 6 && b.subclass == 4 && b.multifunction);
    CHECK(b.primary == 0 && b.secondary == 3 && b.subordinate == 3);
    CHECK(b.command == 6 && b.revision == 3);
    CHECK(b.mem_window_present && b.mem_base == 0x80000000u);
    CHECK(b.mem_limit == 0x8fffffffu);
    CHECK(b.io_window_present && b.io_base == 0x2000u && b.io_limit == 0x3fffu);
    CHECK(b.pref_window_present && b.pref_base == UINT64_C(0x190000000));
    CHECK(b.pref_limit == UINT64_C(0x190ffffff));
    CHECK(b.irq_line == 11 && b.irq_pin == 1);
    CHECK(b.caps == WHP_PCI95_CAPS_COMPLETE && b.pcie_cap_offset == 0x60);
    CHECK(whp_pci95_check_bus_range(&b, 0, 3) == WHP_PCI95_ROUTE_IN_DECLARED_RANGE);
    CHECK(whp_pci95_check_bus_range(&b, 1, 3) == WHP_PCI95_ROUTE_PRIMARY_MISMATCH);
    CHECK(whp_pci95_check_bus_range(&b, 0, 2) == WHP_PCI95_ROUTE_OUTSIDE_RANGE);
    CHECK(whp_pci95_check_bus_range(&b, 0, 256) == WHP_PCI95_ROUTE_BAD_INPUT);
    CHECK(whp_pci95_check_bus_range(0, 0, 3) == WHP_PCI95_ROUTE_BAD_INPUT);
    CHECK(whp_pci95_decode_bridge(cfg, 64, &b) == WHP_PCI95_PARSE_OK);
    CHECK(b.caps == WHP_PCI95_CAPS_TRUNCATED && b.pcie_cap_offset == 0);
    make_bridge(cfg);
    cfg[0x61] = 0x50; /* Cycle. */
    CHECK(whp_pci95_decode_bridge(cfg, 256, &b) == WHP_PCI95_PARSE_OK);
    CHECK(b.caps == WHP_PCI95_CAPS_MALFORMED && b.pcie_cap_offset == 0);
    make_bridge(cfg);
    cfg[0x34] = 0x51; /* Invalid misaligned capability pointer. */
    CHECK(whp_pci95_decode_bridge(cfg, 256, &b) == WHP_PCI95_PARSE_OK);
    CHECK(b.caps == WHP_PCI95_CAPS_MALFORMED);
    make_bridge(cfg);
    put16(cfg, 6, 0); /* No capability advertisement, not a PCIe proof. */
    CHECK(whp_pci95_decode_bridge(cfg, 64, &b) == WHP_PCI95_PARSE_OK);
    CHECK(b.caps == WHP_PCI95_CAPS_NOT_ADVERTISED);
    make_bridge(cfg);
    cfg[0x19] = 0; /* BIOS left child bus unassigned. */
    CHECK(whp_pci95_decode_bridge(cfg, 64, &b) == WHP_PCI95_PARSE_OK);
    CHECK(whp_pci95_check_bus_range(&b, 0, 3) == WHP_PCI95_ROUTE_UNCONFIGURED);
    make_bridge(cfg);
    cfg[0x1a] = 2; /* Subordinate below secondary. */
    CHECK(whp_pci95_decode_bridge(cfg, 64, &b) == WHP_PCI95_PARSE_OK);
    CHECK(whp_pci95_check_bus_range(&b, 0, 3) == WHP_PCI95_ROUTE_UNCONFIGURED);
    make_bridge(cfg);
    put16(cfg, 0, 0xffffu);
    CHECK(whp_pci95_decode_bridge(cfg, 256, &b) == WHP_PCI95_PARSE_NO_DEVICE);
    make_bridge(cfg);
    cfg[14] = 0x80;
    CHECK(whp_pci95_decode_bridge(cfg, 256, &b) == WHP_PCI95_PARSE_NOT_BRIDGE);
    make_bridge(cfg);
    cfg[10] = 0;
    CHECK(whp_pci95_decode_bridge(cfg, 256, &b) == WHP_PCI95_PARSE_NOT_BRIDGE);
    make_bridge(cfg);
    cfg[0x1c] = 0xf0; cfg[0x1d] = 0;
    put16(cfg, 0x20, 0xfff0); put16(cfg, 0x22, 0x0000);
    CHECK(whp_pci95_decode_bridge(cfg, 256, &b) == WHP_PCI95_PARSE_OK);
    CHECK(!b.io_window_present && !b.mem_window_present);
    make_bridge(cfg);
    cfg[0x1c] = 0x21; cfg[0x1d] = 0x31;
    put16(cfg, 0x30, 1); put16(cfg, 0x32, 1);
    CHECK(whp_pci95_decode_bridge(cfg, 256, &b) == WHP_PCI95_PARSE_OK);
    CHECK(b.io_window_present && b.io_base == 0x12000 && b.io_limit == 0x13fff);
    make_bridge(cfg);
    cfg[0x1d] = 0x31; /* Mismatched I/O window formats. */
    CHECK(whp_pci95_decode_bridge(cfg, 256, &b) == WHP_PCI95_PARSE_OK);
    CHECK(!b.io_window_present);
    make_bridge(cfg);
    put16(cfg, 0x26, 0x90f0); /* Mismatched prefetchable format. */
    CHECK(whp_pci95_decode_bridge(cfg, 256, &b) == WHP_PCI95_PARSE_OK);
    CHECK(!b.pref_window_present);
    printf("PCI95 bridge: %d synthetic checks passed\n", checks);
    return 0;
}
