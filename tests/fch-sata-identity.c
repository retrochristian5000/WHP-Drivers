/* Synthetic PCI config snapshots; does not access the actual disk/controller. */
#include "identity.h"
#include <stdio.h>
#include <string.h>

#define CHECK(e) do { ++checks; if (!(e)) { \
    fprintf(stderr, "SATA PCI identity test failed at %d: %s\n", __LINE__, #e); \
    return 1; } } while (0)

static void set16(uint8_t *cfg, size_t at, unsigned int value)
{
    cfg[at] = (uint8_t)value;
    cfg[at + 1u] = (uint8_t)(value >> 8);
}

static void controller(uint8_t *cfg, unsigned int vendor, unsigned int dev,
                       unsigned int sub, unsigned int pi)
{
    memset(cfg, 0, 64u);
    set16(cfg, 0, vendor);
    set16(cfg, 2, dev);
    cfg[0x0bu] = 1u;
    cfg[0x0au] = (uint8_t)sub;
    cfg[9u] = (uint8_t)pi;
}

int main(void)
{
    uint8_t cfg[64];
    struct whp_storage_pci_identity identity;
    struct whp_storage_pci_identity before;
    int checks = 0;
    unsigned int dev;

    memset(&identity, 0x5a, sizeof(identity));
    before = identity;
    CHECK(!whp_storage_decode_pci(0, 64u, &identity));
    CHECK(!whp_storage_decode_pci(cfg, 63u, &identity));
    CHECK(!whp_storage_decode_pci(cfg, 64u, 0));
    CHECK(memcmp(&identity, &before, sizeof(identity)) == 0);
    controller(cfg, 0x1022u, 0x7800u, 1u, 0u);
    CHECK(whp_storage_decode_pci(cfg, sizeof(cfg), &identity));
    CHECK(identity.pci_mode == WHP_STORAGE_MODE_IDE && !identity.mode_disagrees);
    for (dev = 0x7801u; dev <= 0x7805u; ++dev) {
        unsigned int is_ahci = (dev == 0x7801u || dev == 0x7804u);
        controller(cfg, 0x1022u, dev, is_ahci ? 6u : 4u, is_ahci ? 1u : 0u);
        CHECK(whp_storage_decode_pci(cfg, sizeof(cfg), &identity));
        CHECK(identity.amd_fch_id_hint == (is_ahci ? WHP_STORAGE_MODE_AHCI : WHP_STORAGE_MODE_RAID));
        CHECK(!identity.mode_disagrees);
    }
    controller(cfg, 0x1022u, 0x7802u, 6u, 1u);
    CHECK(whp_storage_decode_pci(cfg, sizeof(cfg), &identity));
    CHECK(identity.mode_disagrees && identity.pci_mode == WHP_STORAGE_MODE_AHCI);
    controller(cfg, 0x1234u, 0x5678u, 4u, 0u);
    CHECK(whp_storage_decode_pci(cfg, sizeof(cfg), &identity));
    CHECK(identity.pci_mode == WHP_STORAGE_MODE_RAID && identity.amd_fch_id_hint == WHP_STORAGE_MODE_UNKNOWN);
    controller(cfg, 0x1022u, 0x7801u, 6u, 0u);
    CHECK(whp_storage_decode_pci(cfg, sizeof(cfg), &identity));
    CHECK(identity.pci_mode == WHP_STORAGE_MODE_NON_AHCI_SATA && identity.mode_disagrees);
    controller(cfg, 0x1022u, 0x780cu, 1u, 0u);
    CHECK(whp_storage_decode_pci(cfg, sizeof(cfg), &identity));
    CHECK(identity.amd_fch_id_hint == WHP_STORAGE_MODE_UNKNOWN);
    controller(cfg, 0xffffu, 0x7802u, 4u, 0u);
    before = identity;
    CHECK(!whp_storage_decode_pci(cfg, sizeof(cfg), &identity));
    CHECK(memcmp(&identity, &before, sizeof(identity)) == 0);
    controller(cfg, 0x1022u, 0x7802u, 3u, 0u);
    CHECK(!whp_storage_decode_pci(cfg, sizeof(cfg), &identity));
    CHECK(memcmp(&identity, &before, sizeof(identity)) == 0);
    puts("AMD FCH SATA/RAID: synthetic PCI identity tests passed");
    return 0;
}
