/* WHP-Drivers: USB host-controller identity from a caller-owned PCI snapshot.
 * Identification only: no USB registers, ports, DMA, IRQs, or transfers.
 */
#ifndef WHP_USB_PCI_HCI_H
#define WHP_USB_PCI_HCI_H

#include <stddef.h>
#include <stdint.h>

#define WHP_USB_PCI_HEADER_BYTES 64u

enum whp_usb_hci {
    WHP_USB_HCI_UNKNOWN = 0,
    WHP_USB_HCI_UHCI,
    WHP_USB_HCI_OHCI,
    WHP_USB_HCI_EHCI,
    WHP_USB_HCI_XHCI,
    WHP_USB_HCI_UNSPECIFIED
};

struct whp_usb_pci_hci {
    uint16_t vendor, device, command;
    uint16_t subsystem_vendor, subsystem_device;
    uint8_t revision, prog_if, irq_line, irq_pin;
    enum whp_usb_hci hci;
    enum whp_usb_hci amd_family_id_hint;
    uint8_t id_pi_conflict;
};

/* Only class 0Ch/subclass 03h, type-0 host-controller functions are accepted.
 * Unknown host-controller programming interfaces remain visible, but no driver
 * ABI or supported USB speed may be inferred. Output is unchanged on failure.
 */
int whp_usb_pci_hci_decode(const uint8_t *cfg, size_t length,
                           struct whp_usb_pci_hci *out);

const char *whp_usb_hci_name(enum whp_usb_hci hci);
const char *whp_usb_hci_speed_context(enum whp_usb_hci hci);

#endif
