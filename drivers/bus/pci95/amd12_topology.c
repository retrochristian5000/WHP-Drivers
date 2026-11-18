#include "amd12_topology.h"

static uint16_t read16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

const char *whp_amd12_pcie_port_name(enum whp_amd12_pcie_port port)
{
    switch (port) {
    case WHP_AMD12_PCIE_GFX0: return "GFX0 root port";
    case WHP_AMD12_PCIE_GFX1: return "GFX1 root port";
    case WHP_AMD12_PCIE_GPP0: return "GPP0 root port";
    case WHP_AMD12_PCIE_GPP1: return "GPP1 root port";
    case WHP_AMD12_PCIE_GPP2: return "GPP2 root port";
    case WHP_AMD12_PCIE_GPP3: return "GPP3 root port";
    case WHP_AMD12_PCIE_UMI_FCH: return "UMI bridge to FCH (normally hidden)";
    case WHP_AMD12_PCIE_FCH_GPP0: return "FCH GPP0";
    case WHP_AMD12_PCIE_FCH_GPP1: return "FCH GPP1";
    case WHP_AMD12_PCIE_FCH_GPP2: return "FCH GPP2";
    case WHP_AMD12_PCIE_FCH_GPP3: return "FCH GPP3";
    default: return "unknown";
    }
}

const char *whp_amd12_pcie_origin_name(enum whp_amd12_pcie_origin origin)
{
    switch (origin) {
    case WHP_AMD12_PCIE_APU: return "Family 12h APU";
    case WHP_AMD12_PCIE_FCH: return "Hudson-family FCH";
    default: return "unknown";
    }
}

int whp_amd12_pcie_decode(const uint8_t *cfg, size_t size,
                          unsigned int bus, unsigned int device,
                          unsigned int function, unsigned int pcie_cap_offset,
                          struct whp_amd12_pcie_port_info *out)
{
    struct whp_amd12_pcie_port_info candidate;
    uint16_t vendor, id;
    uint16_t link_status;
    if (cfg == 0 || out == 0 || size < 64u || bus != 0u ||
        cfg[0x0bu] != 0x06u || cfg[0x0au] != 0x04u ||
        (cfg[0x0eu] & 0x7fu) != 1u)
        return 0;
    vendor = read16(cfg);
    id = read16(cfg + 2u);
    if (vendor != 0x1022u)
        return 0;

    if (function == 0u && device >= 2u && device <= 8u &&
        id >= 0x1706u && id <= 0x170du) {
        candidate.origin = WHP_AMD12_PCIE_APU;
        candidate.port = (enum whp_amd12_pcie_port)(WHP_AMD12_PCIE_GFX0 +
                                                    device - 2u);
    } else if (device == 0x15u && function <= 3u &&
               id == (uint16_t)(0x43a0u + function)) {
        candidate.origin = WHP_AMD12_PCIE_FCH;
        candidate.port = (enum whp_amd12_pcie_port)(WHP_AMD12_PCIE_FCH_GPP0 +
                                                    function);
    } else {
        return 0;
    }

    candidate.vendor = vendor;
    candidate.device_id = id;
    candidate.pcie_link_status_valid = 0u;
    candidate.negotiated_speed_code = 0u;
    candidate.negotiated_width = 0u;
    candidate.link_active = 0u;
    /* PCIe capability + 12h is Link Status. Guard the 256-byte
     * standard PCI configuration snapshot; never touch AMD indexed
     * root-port registers to obtain link state.
     */
    if (pcie_cap_offset >= 0x40u && pcie_cap_offset <= 0xecu &&
        pcie_cap_offset + 0x14u <= size &&
        cfg[pcie_cap_offset] == 0x10u) {
        link_status = read16(cfg + pcie_cap_offset + 0x12u);
        candidate.pcie_link_status_valid = 1u;
        candidate.negotiated_speed_code = (uint8_t)(link_status & 0x0fu);
        candidate.negotiated_width = (uint8_t)((link_status >> 4) & 0x3fu);
        candidate.link_active = (uint8_t)((link_status & 0x2000u) != 0u);
    }
    *out = candidate;
    return 1;
}
