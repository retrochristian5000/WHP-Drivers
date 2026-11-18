/* Read-only AMD Family 12h / Hudson PCIe topology evidence.
 * AMD 41131 Rev 3.02, section 2.11, Figure 11 (APU).
 * Hudson-family FCH bridges are independently matched by PCI ID/BDF.
 * A documented port position never proves physical presence or routing.
 */
#ifndef WHP_PCI95_AMD12_TOPOLOGY_H
#define WHP_PCI95_AMD12_TOPOLOGY_H

#include <stddef.h>
#include <stdint.h>

enum whp_amd12_pcie_origin {
    WHP_AMD12_PCIE_APU = 1,
    WHP_AMD12_PCIE_FCH = 2
};

enum whp_amd12_pcie_port {
    WHP_AMD12_PCIE_GFX0 = 1,
    WHP_AMD12_PCIE_GFX1,
    WHP_AMD12_PCIE_GPP0,
    WHP_AMD12_PCIE_GPP1,
    WHP_AMD12_PCIE_GPP2,
    WHP_AMD12_PCIE_GPP3,
    WHP_AMD12_PCIE_UMI_FCH,
    WHP_AMD12_PCIE_FCH_GPP0,
    WHP_AMD12_PCIE_FCH_GPP1,
    WHP_AMD12_PCIE_FCH_GPP2,
    WHP_AMD12_PCIE_FCH_GPP3
};

struct whp_amd12_pcie_port_info {
    enum whp_amd12_pcie_origin origin;
    enum whp_amd12_pcie_port port;
    uint16_t vendor, device_id;
    uint8_t pcie_link_status_valid;
    uint8_t negotiated_speed_code;
    uint8_t negotiated_width;
    uint8_t link_active;
};

/* Uses a PCI type-1 bridge header and externally established PCIe capability
 * pointer. Does not scan, map, or write PCI configuration registers.
 * The FCH GPP IDs are family candidates, not confirmation of board wiring.
 * Return 0 on unsupported identity, preserving output unchanged.
 */
int whp_amd12_pcie_decode(const uint8_t *cfg, size_t size,
                          unsigned int bus, unsigned int device,
                          unsigned int function, unsigned int pcie_cap_offset,
                          struct whp_amd12_pcie_port_info *out);

const char *whp_amd12_pcie_port_name(enum whp_amd12_pcie_port port);
const char *whp_amd12_pcie_origin_name(enum whp_amd12_pcie_origin origin);

#endif
