/* Named, read-only PCI configuration-register evidence for WHPPROBE.
 * The caller supplies a complete 64- or 256-byte BIOS snapshot.
 * This never accesses I/O ports, MMIO, or PCI hardware itself.
 */
#ifndef WHP_PCI95_CONFIG_REPORT_H
#define WHP_PCI95_CONFIG_REPORT_H
#include <stddef.h>
#include <stdint.h>

enum whp_pci_cap_read_status {
    WHP_PCI_CAP_NOT_ADVERTISED = 0,
    WHP_PCI_CAP_HEADER_ONLY,
    WHP_PCI_CAP_COMPLETE,
    WHP_PCI_CAP_MALFORMED
};

struct whp_pci_config_report {
    uint16_t vendor, device, command, status;
    uint8_t revision, prog_if, subclass, class_code;
    uint8_t cache_line_size, latency_timer, header_type, bist;
    uint8_t irq_line, irq_pin;
    uint8_t layout;                 /* 0: endpoint; 1: bridge; otherwise unknown. */
    uint32_t bars[6];              /* Raw registers, not decoded address sizes. */
    uint8_t bar_count;
    uint16_t subsystem_vendor, subsystem_device; /* Type 0 only. */
    uint8_t primary, secondary, subordinate, secondary_latency; /* Type 1 only. */
    uint16_t bridge_control;       /* Type 1 only. */
    uint8_t caps_pointer;
    enum whp_pci_cap_read_status caps_status;
    uint8_t pcie_cap_offset;       /* Only when the entire chain is valid. */
    uint16_t pcie_flags;
    uint8_t pcie_link_status_valid;
    uint16_t pcie_link_status;
};

/* Return 1 for a valid, present function; otherwise leave *out unchanged.
 * A 64-byte snapshot can identify the capability pointer, but cannot prove
 * capability contents. Do not interpret absent bytes as zero registers.
 */
int whp_pci_config_report_decode(const uint8_t *cfg, size_t length,
                                 struct whp_pci_config_report *out);
#endif
