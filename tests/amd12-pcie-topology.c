/* Pure, fabricated PCI snapshots; never reads hardware registers. */
#include "amd12_topology.h"
#include <stdio.h>
#include <string.h>

#define CHECK(e) do { ++checks; if (!(e)) { \
    fprintf(stderr, "AMD12 topology test line %d: %s\n", __LINE__, #e); \
    return 1; } } while (0)

static void put16(uint8_t *p, unsigned int off, unsigned int value)
{
    p[off] = (uint8_t)value;
    p[off + 1u] = (uint8_t)(value >> 8);
}
static void fixture(uint8_t *cfg, unsigned int vendor, unsigned int id)
{
    memset(cfg, 0, 256u);
    put16(cfg, 0u, vendor);
    put16(cfg, 2u, id);
    cfg[0x0bu] = 0x06u;
    cfg[0x0au] = 0x04u;
    cfg[0x0eu] = 0x01u;
}
int main(void)
{
    uint8_t cfg[256];
    struct whp_amd12_pcie_port_info out, prior;
    unsigned int dev, fn;
    int checks = 0;
    memset(&out, 0x5a, sizeof(out));
    prior = out;
    CHECK(!whp_amd12_pcie_decode(0, 256u, 0u, 4u, 0u, 0u, &out));
    CHECK(!whp_amd12_pcie_decode(cfg, 63u, 0u, 4u, 0u, 0u, &out));
    CHECK(memcmp(&out, &prior, sizeof(out)) == 0);
    for (dev = 2u; dev <= 8u; ++dev) {
        fixture(cfg, 0x1022u, 0x1709u);
        CHECK(whp_amd12_pcie_decode(cfg, sizeof(cfg), 0u, dev, 0u,
                                    0u, &out));
        CHECK(out.origin == WHP_AMD12_PCIE_APU);
        CHECK(out.port == (enum whp_amd12_pcie_port)
                          (WHP_AMD12_PCIE_GFX0 + dev - 2u));
        CHECK(!out.pcie_link_status_valid);
    }
    for (fn = 0u; fn < 4u; ++fn) {
        fixture(cfg, 0x1022u, 0x43a0u + fn);
        CHECK(whp_amd12_pcie_decode(cfg, sizeof(cfg), 0u, 0x15u,
                                    fn, 0u, &out));
        CHECK(out.origin == WHP_AMD12_PCIE_FCH);
        CHECK(out.port == (enum whp_amd12_pcie_port)
                          (WHP_AMD12_PCIE_FCH_GPP0 + fn));
    }
    fixture(cfg, 0x1022u, 0x1709u);
    cfg[0x50u] = 0x10u;  /* Standard PCIe capability. */
    put16(cfg, 0x62u, 0x2082u); /* Gen2 5 GT/s, x8, link active. */
    CHECK(whp_amd12_pcie_decode(cfg, sizeof(cfg), 0u, 4u, 0u,
                                0x50u, &out));
    CHECK(out.pcie_link_status_valid && out.negotiated_speed_code == 2u);
    CHECK(out.negotiated_width == 8u && out.link_active);
    CHECK(whp_amd12_pcie_decode(cfg, 64u, 0u, 4u, 0u, 0x50u, &out));
    CHECK(!out.pcie_link_status_valid);
    cfg[0x50u] = 0x05u; /* MSI is not PCIe capability. */
    CHECK(whp_amd12_pcie_decode(cfg, sizeof(cfg), 0u, 4u, 0u,
                                0x50u, &out));
    CHECK(!out.pcie_link_status_valid);
    prior = out;
    fixture(cfg, 0x1234u, 0x1709u);
    CHECK(!whp_amd12_pcie_decode(cfg, sizeof(cfg), 0u, 4u, 0u,
                                 0u, &out));
    CHECK(memcmp(&out, &prior, sizeof(out)) == 0);
    fixture(cfg, 0x1022u, 0x43a1u);
    CHECK(!whp_amd12_pcie_decode(cfg, sizeof(cfg), 0u, 0x15u, 0u,
                                 0u, &out)); /* Function/ID mismatch. */
    fixture(cfg, 0x1022u, 0x1709u);
    CHECK(!whp_amd12_pcie_decode(cfg, sizeof(cfg), 1u, 4u, 0u,
                                 0u, &out));
    cfg[0x0eu] = 0x00u;
    CHECK(!whp_amd12_pcie_decode(cfg, sizeof(cfg), 0u, 4u, 0u,
                                 0u, &out));
    puts("AMD Family 12h PCIe root-port topology tests passed");
    return 0;
}
