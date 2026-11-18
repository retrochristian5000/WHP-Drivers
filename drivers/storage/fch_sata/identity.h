/* Read-only PCI identity for AMD FCH SATA controller modes.
 * Does not detect disks, a RAID volume, or driver readiness.
 */
#ifndef WHP_FCH_SATA_IDENTITY_H
#define WHP_FCH_SATA_IDENTITY_H
#include <stddef.h>
#include <stdint.h>

#define WHP_FCH_SATA_PCI_HEADER_BYTES 64u

enum whp_storage_pci_mode {
    WHP_STORAGE_MODE_UNKNOWN = 0,
    WHP_STORAGE_MODE_IDE,
    WHP_STORAGE_MODE_AHCI,
    WHP_STORAGE_MODE_RAID,
    WHP_STORAGE_MODE_NON_AHCI_SATA
};

struct whp_storage_pci_identity {
    uint16_t vendor, device, command, subsystem_vendor, subsystem_device;
    uint8_t revision, subclass, prog_if, irq_line, irq_pin;
    enum whp_storage_pci_mode pci_mode;
    enum whp_storage_pci_mode amd_fch_id_hint;
    uint8_t mode_disagrees;
};

/* Returns 1 for a PCI storage IDE/SATA/RAID controller, 0 otherwise.
 * Output remains unchanged on failure. No PCI access or hardware writes.
 */
int whp_storage_decode_pci(const uint8_t *cfg, size_t length,
                            struct whp_storage_pci_identity *out);
const char *whp_storage_mode_name(enum whp_storage_pci_mode mode);
#endif
