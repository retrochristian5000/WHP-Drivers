/* Host-only PCI register evidence tests: synthetic snapshots, no hardware. */
#include "pci_config_report.h"
#include <stdio.h>
#include <string.h>

#define CHECK(e) do { ++checks; if (!(e)) { \
    fprintf(stderr, "PCI config test failed line %d: %s\n", \
            __LINE__, #e); return 1; } } while (0)

static void put16(uint8_t *cfg, unsigned int at, uint16_t value)
{
    cfg[at] = (uint8_t)value;
    cfg[at + 1u] = (uint8_t)(value >> 8);
}

int main(void)
{
    uint8_t cfg[256];
    struct whp_pci_config_report r;
    int checks = 0;

    memset(cfg, 0, sizeof(cfg));
    put16(cfg, 0u, 0x10ecu);
    put16(cfg, 2u, 0x8168u);
    put16(cfg, 4u, 0x0007u);
    put16(cfg, 6u, 0x0010u);
    cfg[8] = 0x06u; cfg[9] = 0; cfg[10] = 0; cfg[11] = 2u;
    cfg[0x0e] = 0x80u; /* Multi-function endpoint; layout is type 0. */
    cfg[0x34] = 0x40u;
    put16(cfg, 0x2cu, 0x1022u);
    put16(cfg, 0x2eu, 0x1234u);

    memset(&r, 0xa5, sizeof(r));
    CHECK(!whp_pci_config_report_decode(0, 64u, &r));
    CHECK(!whp_pci_config_report_decode(cfg, 63u, &r));
    CHECK(!whp_pci_config_report_decode(cfg, 65u, &r));
    CHECK(!whp_pci_config_report_decode(cfg, 64u, 0));
    CHECK((unsigned int)r.vendor == 0xa5a5u); /* Failure preserves output. */

    CHECK(whp_pci_config_report_decode(cfg, 64u, &r));
    CHECK(r.vendor == 0x10ecu && r.device == 0x8168u);
    CHECK(r.command == 7u && r.status == 0x10u);
    CHECK(r.class_code == 2u && r.revision == 6u);
    CHECK(r.layout == 0u && r.header_type == 0x80u && r.bar_count == 6u);
    CHECK(r.subsystem_vendor == 0x1022u && r.subsystem_device == 0x1234u);
    CHECK(r.caps_pointer == 0x40u && r.caps_status == WHP_PCI_CAP_HEADER_ONLY);
    CHECK(r.pcie_cap_offset == 0u && !r.pcie_link_status_valid);

    cfg[0x40] = 0x01u; cfg[0x41] = 0x48u; /* Power management -> PCIe. */
    cfg[0x48] = 0x10u; cfg[0x49] = 0u;
    put16(cfg, 0x4au, 0u);               /* PCIe endpoint type. */
    put16(cfg, 0x5au, 0x2042u);          /* DLL active, width x4, speed code 2. */
    CHECK(whp_pci_config_report_decode(cfg, 256u, &r));
    CHECK(r.caps_status == WHP_PCI_CAP_COMPLETE && r.pcie_cap_offset == 0x48u);
    CHECK(r.pcie_link_status_valid && r.pcie_link_status == 0x2042u);

    cfg[0x49] = 0x40u;                   /* Cycle: not a valid chain. */
    CHECK(whp_pci_config_report_decode(cfg, 256u, &r));
    CHECK(r.caps_status == WHP_PCI_CAP_MALFORMED && r.pcie_cap_offset == 0u);
    cfg[0x49] = 0u;
    put16(cfg, 0x4au, 9u << 4);          /* RC integrated endpoint: no link. */
    CHECK(whp_pci_config_report_decode(cfg, 256u, &r));
    CHECK(r.pcie_cap_offset == 0x48u && !r.pcie_link_status_valid);

    cfg[0x0e] = 1u;                      /* Type 1 is not a type 0 subsystem. */
    cfg[0x18] = 0u; cfg[0x19] = 2u; cfg[0x1a] = 5u;
    put16(cfg, 0x3eu, 0x0101u);
    CHECK(whp_pci_config_report_decode(cfg, 64u, &r));
    CHECK(r.layout == 1u && r.bar_count == 2u);
    CHECK(r.secondary == 2u && r.subordinate == 5u && r.bridge_control == 0x0101u);
    CHECK(r.subsystem_vendor == 0u);    /* Never read endpoint-only field. */

    put16(cfg, 6u, 0u);                 /* No capabilities advertised. */
    CHECK(whp_pci_config_report_decode(cfg, 256u, &r));
    CHECK(r.caps_status == WHP_PCI_CAP_NOT_ADVERTISED);
    put16(cfg, 0u, 0xffffu);
    CHECK(!whp_pci_config_report_decode(cfg, 64u, &r));
    CHECK(r.vendor == 0x10ecu);         /* No mutation on absent function. */

    printf("PCI config standard register report: %d synthetic checks passed\n", checks);
    return 0;
}
