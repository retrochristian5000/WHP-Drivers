/* RTL8111E identification core for the WHP-Drivers Win95 RTM project.
 * Data-only: no PCI enumeration, BAR mapping, MMIO, IRQ, DMA or NDIS binding.
 */
#ifndef WHP_RTL8111E_H
#define WHP_RTL8111E_H

#include <stddef.h>
#include <stdint.h>

#define WHP_RTL8111E_PCI_VENDOR        0x10ecu
#define WHP_RTL8111E_PCI_DEVICE        0x8168u
#define WHP_RTL8111E_PCI_CLASS         0x02u
#define WHP_RTL8111E_PCI_SUBCLASS      0x00u
#define WHP_RTL8111E_TXCONFIG_OFFSET  0x40u
#define WHP_RTL8111E_PCI_HEADER_BYTES 64u
#define WHP_RTL8111E_PCI_CAP_BYTES    256u

#define WHP_PCI_COMMAND_IO_SPACE       0x0001u
#define WHP_PCI_COMMAND_MEMORY_SPACE   0x0002u
#define WHP_PCI_COMMAND_BUS_MASTER     0x0004u

struct whp_rtl8111e_pci_id {
    uint16_t vendor;
    uint16_t device;
    uint8_t class_code;
    uint8_t subclass;
};

enum whp_rtl8111e_identity {
    WHP_RTL8111E_NO_MATCH = 0,
    WHP_RTL8111E_FAMILY_CANDIDATE,
    WHP_RTL8111E_E1_CANDIDATE,
    WHP_RTL8111E_E2_CANDIDATE,
    WHP_RTL8111E_EVL_CANDIDATE
};

/* Capability status is separate from PCI identity. A 64-byte header
 * cannot establish that the PCIe capability is absent.
 */
enum whp_rtl8111e_pci_caps_state {
    WHP_PCI_CAPS_NOT_ADVERTISED = 0,
    WHP_PCI_CAPS_COMPLETE,
    WHP_PCI_CAPS_TRUNCATED,
    WHP_PCI_CAPS_MALFORMED
};

enum whp_rtl8111e_pci_snapshot_result {
    WHP_PCI_SNAPSHOT_OK = 0,
    WHP_PCI_SNAPSHOT_BAD_INPUT,
    WHP_PCI_SNAPSHOT_NO_DEVICE,
    WHP_PCI_SNAPSHOT_NOT_ENDPOINT
};

struct whp_rtl8111e_pci_snapshot {
    struct whp_rtl8111e_pci_id id;
    enum whp_rtl8111e_identity family;
    uint16_t command;
    uint16_t status;
    uint16_t subsystem_vendor;
    uint16_t subsystem_device;
    uint32_t raw_bar[6]; /* Raw values: not mapped physical addresses. */
    uint8_t pci_revision;
    uint8_t programming_interface;
    uint8_t header_type;
    uint8_t multifunction;
    uint8_t irq_line;
    uint8_t irq_pin;
    enum whp_rtl8111e_pci_caps_state capabilities;
    uint8_t pcie_cap_offset; /* Zero means not established. */
    uint8_t pm_cap_offset;
    uint8_t msi_cap_offset;
    uint8_t msix_cap_offset;
};

/* Decode caller-supplied RAW little-endian PCI configuration bytes.
 * Require at least 64 bytes; optionally inspect standard capabilities
 * within the first 256 bytes. No hardware or OS calls are made.
 * Decode only type-0 endpoints. On non-OK results, output is unchanged.
 * Valid capability offsets require capabilities == WHP_PCI_CAPS_COMPLETE.
 * An OK result does not establish Win95 RTM PCIe enumeration support.
 */
enum whp_rtl8111e_pci_snapshot_result
whp_rtl8111e_decode_pci_snapshot(const uint8_t *cfg, size_t length,
                                 struct whp_rtl8111e_pci_snapshot *snapshot);

/* 10EC:8168 is shared by many non-E variants; this is NOT E confirmation. */
enum whp_rtl8111e_identity
whp_rtl8111e_identify_pci(const struct whp_rtl8111e_pci_id *id);

/* Only call with a TxConfig register snapshot (offset 0x40) collected by
 * separately audited code. This function never reads device registers.
 * The E1/E2/EVL results remain candidates pending machine verification.
 */
enum whp_rtl8111e_identity
whp_rtl8111e_identify_txconfig(const struct whp_rtl8111e_pci_id *id,
                               uint32_t txconfig);

/* Validate six already-read address bytes; zero/multicast/broadcast rejected.
 * No device access or changes to station address are performed here.
 */
int whp_rtl8111e_mac_valid(const uint8_t mac[6]);

#endif
