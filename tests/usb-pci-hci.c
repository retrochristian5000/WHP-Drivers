/* Fabricated PCI headers only; no physical USB hardware is accessed. */
#include "pci_hci.h"
#include <stdio.h>
#include <string.h>

#define CHECK(e) do { ++checks; if (!(e)) { \
    fprintf(stderr, "USB HCI regression failed at line %d: %s\n", \
            __LINE__, #e); return 1; } } while (0)

static void put16(uint8_t *p, unsigned int offset, unsigned int value)
{
    p[offset] = (uint8_t)value;
    p[offset + 1u] = (uint8_t)(value >> 8);
}

static void setup(uint8_t *cfg, unsigned int vendor, unsigned int device,
                  unsigned int class_code, unsigned int subclass,
                  unsigned int prog_if)
{
    memset(cfg, 0, 64u);
    put16(cfg, 0u, vendor);
    put16(cfg, 2u, device);
    cfg[0x0bu] = (uint8_t)class_code;
    cfg[0x0au] = (uint8_t)subclass;
    cfg[9u] = (uint8_t)prog_if;
}

int main(void)
{
    uint8_t cfg[64];
    struct whp_usb_pci_hci info, prior;
    int checks = 0;
    unsigned int i;
    static const unsigned int amd_dev[] = {
        0x7807u, 0x7808u, 0x7809u, 0x7812u, 0x7814u
    };
    static const unsigned int pi[] = {
        0x10u, 0x20u, 0x10u, 0x30u, 0x30u
    };
    static const enum whp_usb_hci expected[] = {
        WHP_USB_HCI_OHCI, WHP_USB_HCI_EHCI, WHP_USB_HCI_OHCI,
        WHP_USB_HCI_XHCI, WHP_USB_HCI_XHCI
    };
    memset(&info, 0x5au, sizeof(info));
    prior = info;
    CHECK(!whp_usb_pci_hci_decode(0, 64u, &info));
    CHECK(!whp_usb_pci_hci_decode(cfg, 63u, &info));
    CHECK(!whp_usb_pci_hci_decode(cfg, 64u, 0));
    CHECK(memcmp(&info, &prior, sizeof(info)) == 0);
    for (i = 0u; i < sizeof(amd_dev) / sizeof(amd_dev[0]); ++i) {
        setup(cfg, 0x1022u, amd_dev[i], 0x0cu, 0x03u, pi[i]);
        CHECK(whp_usb_pci_hci_decode(cfg, sizeof(cfg), &info));
        CHECK(info.hci == expected[i] &&
              info.amd_family_id_hint == expected[i] &&
              !info.id_pi_conflict);
    }
    setup(cfg, 0x8086u, 0x7112u, 0x0cu, 0x03u, 0x00u);
    CHECK(whp_usb_pci_hci_decode(cfg, sizeof(cfg), &info));
    CHECK(info.hci == WHP_USB_HCI_UHCI &&
          info.amd_family_id_hint == WHP_USB_HCI_UNKNOWN);
    setup(cfg, 0x1234u, 0x5678u, 0x0cu, 0x03u, 0x80u);
    CHECK(whp_usb_pci_hci_decode(cfg, sizeof(cfg), &info));
    CHECK(info.hci == WHP_USB_HCI_UNSPECIFIED);
    setup(cfg, 0x1234u, 0x5678u, 0x0cu, 0x03u, 0x40u);
    CHECK(whp_usb_pci_hci_decode(cfg, sizeof(cfg), &info));
    CHECK(info.hci == WHP_USB_HCI_UNKNOWN);
    setup(cfg, 0x1022u, 0x7808u, 0x0cu, 0x03u, 0x10u);
    CHECK(whp_usb_pci_hci_decode(cfg, sizeof(cfg), &info));
    CHECK(info.hci == WHP_USB_HCI_OHCI && info.id_pi_conflict);
    /* 780Bh is commonly identified as SMBus; do not infer USB from ID. */
    setup(cfg, 0x1022u, 0x780bu, 0x0cu, 0x05u, 0x00u);
    prior = info;
    CHECK(!whp_usb_pci_hci_decode(cfg, sizeof(cfg), &info));
    CHECK(memcmp(&info, &prior, sizeof(info)) == 0);
    setup(cfg, 0x1234u, 0x5678u, 0x0cu, 0x03u, 0xfeu);
    CHECK(!whp_usb_pci_hci_decode(cfg, sizeof(cfg), &info));
    setup(cfg, 0xffffu, 0x0001u, 0x0cu, 0x03u, 0x20u);
    CHECK(!whp_usb_pci_hci_decode(cfg, sizeof(cfg), &info));
    setup(cfg, 0x1234u, 0x5678u, 0x0cu, 0x03u, 0x20u);
    cfg[0x0eu] = 1u; /* Type-1 bridge cannot be treated as type-0 HCI. */
    CHECK(!whp_usb_pci_hci_decode(cfg, sizeof(cfg), &info));
    puts("USB UHCI/OHCI/EHCI/xHCI: synthetic PCI identity checks passed");
    return 0;
}
