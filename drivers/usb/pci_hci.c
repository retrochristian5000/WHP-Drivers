#include "pci_hci.h"

static uint16_t whp_usb_read16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static enum whp_usb_hci whp_usb_from_pi(unsigned int prog_if)
{
    switch (prog_if) {
    case 0x00u: return WHP_USB_HCI_UHCI;
    case 0x10u: return WHP_USB_HCI_OHCI;
    case 0x20u: return WHP_USB_HCI_EHCI;
    case 0x30u: return WHP_USB_HCI_XHCI;
    case 0x80u: return WHP_USB_HCI_UNSPECIFIED;
    default: return WHP_USB_HCI_UNKNOWN;
    }
}

const char *whp_usb_hci_name(enum whp_usb_hci hci)
{
    switch (hci) {
    case WHP_USB_HCI_UHCI: return "UHCI";
    case WHP_USB_HCI_OHCI: return "OHCI";
    case WHP_USB_HCI_EHCI: return "EHCI";
    case WHP_USB_HCI_XHCI: return "xHCI";
    case WHP_USB_HCI_UNSPECIFIED: return "unspecified HCI";
    default: return "unknown HCI";
    }
}

const char *whp_usb_hci_speed_context(enum whp_usb_hci hci)
{
    switch (hci) {
    case WHP_USB_HCI_UHCI:
    case WHP_USB_HCI_OHCI:
        return "USB 1.x low/full-speed path (1.5/12 Mb/s)";
    case WHP_USB_HCI_EHCI:
        return "USB 2.0 high-speed path (480 Mb/s); slower devices need companions/TT";
    case WHP_USB_HCI_XHCI:
        return "xHCI architecture; actual USB speed capabilities unverified";
    default:
        return "supported speeds unknown";
    }
}

int whp_usb_pci_hci_decode(const uint8_t *cfg, size_t length,
                           struct whp_usb_pci_hci *out)
{
    struct whp_usb_pci_hci candidate;
    if (cfg == 0 || out == 0 || length < WHP_USB_PCI_HEADER_BYTES)
        return 0;
    candidate.vendor = whp_usb_read16(cfg);
    candidate.device = whp_usb_read16(cfg + 2u);
    if (candidate.vendor == 0xffffu || candidate.vendor == 0u ||
        cfg[0x0bu] != 0x0cu || cfg[0x0au] != 0x03u ||
        (cfg[0x0eu] & 0x7fu) != 0u || cfg[9u] == 0xfeu)
        return 0; /* 0Ch:03h:FEh is a USB device, not a host controller. */

    candidate.prog_if = cfg[9u];
    candidate.hci = whp_usb_from_pi(candidate.prog_if);
    candidate.amd_family_id_hint = WHP_USB_HCI_UNKNOWN;
    if (candidate.vendor == 0x1022u) {
        switch (candidate.device) {
        case 0x7807u:
        case 0x7809u:
            candidate.amd_family_id_hint = WHP_USB_HCI_OHCI;
            break;
        case 0x7808u:
            candidate.amd_family_id_hint = WHP_USB_HCI_EHCI;
            break;
        case 0x7812u:
        case 0x7814u:
            candidate.amd_family_id_hint = WHP_USB_HCI_XHCI;
            break;
        default:
            break; /* Other families require explicit evidence. */
        }
    }
    candidate.command = whp_usb_read16(cfg + 4u);
    candidate.revision = cfg[8u];
    candidate.subsystem_vendor = whp_usb_read16(cfg + 0x2cu);
    candidate.subsystem_device = whp_usb_read16(cfg + 0x2eu);
    candidate.irq_line = cfg[0x3cu];
    candidate.irq_pin = cfg[0x3du];
    candidate.id_pi_conflict = (uint8_t)(
        candidate.amd_family_id_hint != WHP_USB_HCI_UNKNOWN &&
        candidate.amd_family_id_hint != candidate.hci);
    *out = candidate;
    return 1;
}
