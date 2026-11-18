/* WHP-Drivers: hardware-independent HD Audio controller evidence.
 * PCI class/device IDs are NOT HDA codec identities.
 * No PCI enumeration, MMIO reads, IRQ/DMA, codec verbs, or reset here.
 */
#ifndef WHP_HDA_PROBE_H
#define WHP_HDA_PROBE_H
#include <stddef.h>
#include <stdint.h>

#define WHP_HDA_PCI_CLASS       0x04u
#define WHP_HDA_PCI_SUBCLASS    0x03u
#define WHP_HDA_PCI_PROGIF     0x00u
#define WHP_HDA_AMD_VENDOR      0x1022u
#define WHP_HDA_FCH_AZALIA_ID  0x780du
#define WHP_HDA_PCI_BYTES      64u
#define WHP_HDA_MMIO_BYTES     96u

enum whp_hda_pci_result {
    WHP_HDA_PCI_BAD_INPUT = 0,
    WHP_HDA_PCI_NO_DEVICE,
    WHP_HDA_PCI_NOT_HDA,
    WHP_HDA_PCI_CLASS_CANDIDATE,
    WHP_HDA_PCI_AMD_FCH_CANDIDATE
};

enum whp_hda_bar_kind {
    WHP_HDA_BAR_UNASSIGNED = 0,
    WHP_HDA_BAR_MMIO32,
    WHP_HDA_BAR_MMIO64,
    WHP_HDA_BAR_NOT_MEMORY,
    WHP_HDA_BAR_INVALID
};

struct whp_hda_pci {
    uint16_t vendor, device, command, status;
    uint16_t subsystem_vendor, subsystem_device;
    uint8_t pci_revision, programming_interface;
    uint8_t irq_line, irq_pin, multifunction;
    uint32_t bar0_raw, bar1_raw;
    uint64_t mmio_base; /* Raw decoded BAR; not a valid OS mapping. */
    enum whp_hda_bar_kind bar_kind;
};

/* PCI type-0 config header bytes, as captured by a separate read-only
 * enumerator. Class 04:03 PI=00 is an HDA-compatible controller candidate.
 * AMD 1022:780d is an FCH Azalia *candidate*, never model proof.
 * Output unchanged on BAD_INPUT/NO_DEVICE/NOT_HDA.
 */
enum whp_hda_pci_result whp_hda_decode_pci(
    const uint8_t *config, size_t length, struct whp_hda_pci *out);

enum whp_hda_regs_result {
    WHP_HDA_REGS_BAD_INPUT = 0,
    WHP_HDA_REGS_UNSUPPORTED_VERSION,
    WHP_HDA_REGS_OK
};

struct whp_hda_regs {
    uint16_t gcap, statests;
    uint32_t gctl;
    uint8_t major, minor;
    uint8_t output_streams, input_streams, bidir_streams;
    uint8_t serial_data_outputs;
    uint8_t supports_64bit;
    uint8_t corb_supported_sizes; /* bits 0..2 = 2, 16, 256 entries */
    uint8_t rirb_supported_sizes;
};

/* Decode an externally supplied >=96-byte *memory-mapped HDA register*
 * snapshot. These are NOT PCI configuration bytes and must not be
 * obtained by blindly dereferencing BAR0. Registers include W1C bits;
 * this function does not clear them or write anything.
 * Output unchanged on invalid or unsupported-version snapshots.
 * A positive result does not prove a codec is present or playing audio.
 */
enum whp_hda_regs_result whp_hda_decode_regs(
    const uint8_t *regs, size_t length, struct whp_hda_regs *out);
#endif
