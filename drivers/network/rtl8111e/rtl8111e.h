/* RTL8111E identification core for the WHP-Drivers Win95 RTM project.
 * Data-only: no PCI enumeration, BAR mapping, MMIO, IRQ, DMA or NDIS binding.
 */
#ifndef WHP_RTL8111E_H
#define WHP_RTL8111E_H

#include <stdint.h>

#define WHP_RTL8111E_PCI_VENDOR        0x10ecu
#define WHP_RTL8111E_PCI_DEVICE        0x8168u
#define WHP_RTL8111E_PCI_CLASS         0x02u
#define WHP_RTL8111E_PCI_SUBCLASS      0x00u
#define WHP_RTL8111E_TXCONFIG_OFFSET  0x40u

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
