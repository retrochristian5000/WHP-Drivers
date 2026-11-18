#include "identity.h"

static uint16_t read16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

const char *whp_storage_mode_name(enum whp_storage_pci_mode mode)
{
    switch (mode) {
    case WHP_STORAGE_MODE_IDE: return "IDE";
    case WHP_STORAGE_MODE_AHCI: return "AHCI";
    case WHP_STORAGE_MODE_RAID: return "RAID";
    case WHP_STORAGE_MODE_NON_AHCI_SATA: return "SATA (non-AHCI PI)";
    default: return "unknown";
    }
}

int whp_storage_decode_pci(const uint8_t *cfg, size_t length,
                            struct whp_storage_pci_identity *out)
{
    struct whp_storage_pci_identity candidate;
    if (cfg == 0 || out == 0 || length < WHP_FCH_SATA_PCI_HEADER_BYTES)
        return 0;
    candidate.vendor = read16(cfg);
    candidate.device = read16(cfg + 2u);
    if (candidate.vendor == 0xffffu || cfg[0x0bu] != 0x01u)
        return 0;
    candidate.subclass = cfg[0x0au];
    candidate.prog_if = cfg[0x09u];
    if (candidate.subclass == 0x01u)
        candidate.pci_mode = WHP_STORAGE_MODE_IDE;
    else if (candidate.subclass == 0x04u)
        candidate.pci_mode = WHP_STORAGE_MODE_RAID;
    else if (candidate.subclass == 0x06u)
        candidate.pci_mode = candidate.prog_if == 0x01u ?
            WHP_STORAGE_MODE_AHCI : WHP_STORAGE_MODE_NON_AHCI_SATA;
    else
        return 0;

    candidate.amd_fch_id_hint = WHP_STORAGE_MODE_UNKNOWN;
    if (candidate.vendor == 0x1022u) {
        switch (candidate.device) {
        case 0x7800u: candidate.amd_fch_id_hint = WHP_STORAGE_MODE_IDE; break;
        case 0x7801u:
        case 0x7804u: candidate.amd_fch_id_hint = WHP_STORAGE_MODE_AHCI; break;
        case 0x7802u:
        case 0x7803u:
        case 0x7805u: candidate.amd_fch_id_hint = WHP_STORAGE_MODE_RAID; break;
        default: break;
        }
    }
    candidate.command = read16(cfg + 4u);
    candidate.revision = cfg[8u];
    candidate.subsystem_vendor = read16(cfg + 0x2cu);
    candidate.subsystem_device = read16(cfg + 0x2eu);
    candidate.irq_line = cfg[0x3cu];
    candidate.irq_pin = cfg[0x3du];
    /* A mode disagreement is an investigation clue, never a driver binding. */
    candidate.mode_disagrees = (uint8_t)(
        candidate.amd_fch_id_hint != WHP_STORAGE_MODE_UNKNOWN &&
        candidate.pci_mode != candidate.amd_fch_id_hint);
    *out = candidate;
    return 1;
}
