#include "hda_probe.h"

static uint16_t read16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}
static uint32_t read32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

enum whp_hda_pci_result whp_hda_decode_pci(
    const uint8_t *config, size_t length, struct whp_hda_pci *out)
{
    struct whp_hda_pci s = {0};
    uint32_t base;
    if (!config || !out || length < WHP_HDA_PCI_BYTES)
        return WHP_HDA_PCI_BAD_INPUT;
    s.vendor = read16(config);
    s.device = read16(config + 2u);
    if (s.vendor == 0xffffu || s.vendor == 0u)
        return WHP_HDA_PCI_NO_DEVICE;
    /* A bridge/header type 1 is not a type-0 audio endpoint. */
    if ((config[0x0eu] & 0x7fu) != 0u ||
        config[0x0bu] != WHP_HDA_PCI_CLASS ||
        config[0x0au] != WHP_HDA_PCI_SUBCLASS ||
        config[0x09u] != WHP_HDA_PCI_PROGIF)
        return WHP_HDA_PCI_NOT_HDA;

    s.command = read16(config + 4u);
    s.status = read16(config + 6u);
    s.pci_revision = config[8u];
    s.programming_interface = config[9u];
    s.multifunction = (uint8_t)((config[0xeu] & 0x80u) != 0u);
    s.subsystem_vendor = read16(config + 0x2cu);
    s.subsystem_device = read16(config + 0x2eu);
    s.irq_line = config[0x3cu];
    s.irq_pin = config[0x3du];
    s.bar0_raw = read32(config + 0x10u);
    s.bar1_raw = read32(config + 0x14u);
    /* A BAR is only a firmware/OS assignment hint. Never access it here. */
    if (s.bar0_raw == 0u || s.bar0_raw == UINT32_MAX) {
        s.bar_kind = WHP_HDA_BAR_UNASSIGNED;
    } else if (s.bar0_raw & 1u) {
        s.bar_kind = WHP_HDA_BAR_NOT_MEMORY;
    } else if ((s.bar0_raw & 0x6u) == 0u) {
        s.mmio_base = (uint64_t)(s.bar0_raw & UINT32_C(0xfffffff0));
        s.bar_kind = s.mmio_base ? WHP_HDA_BAR_MMIO32 :
                                   WHP_HDA_BAR_UNASSIGNED;
    } else if ((s.bar0_raw & 0x6u) == 0x4u) {
        s.mmio_base = ((uint64_t)s.bar1_raw << 32) |
                      (uint64_t)(s.bar0_raw & UINT32_C(0xfffffff0));
        s.bar_kind = s.mmio_base ? WHP_HDA_BAR_MMIO64 :
                                   WHP_HDA_BAR_UNASSIGNED;
    } else {
        s.bar_kind = WHP_HDA_BAR_INVALID;
    }
    *out = s;
    if (s.vendor == WHP_HDA_AMD_VENDOR &&
        s.device == WHP_HDA_FCH_AZALIA_ID)
        return WHP_HDA_PCI_AMD_FCH_CANDIDATE;
    return WHP_HDA_PCI_CLASS_CANDIDATE;
}

enum whp_hda_regs_result whp_hda_decode_regs(
    const uint8_t *regs, size_t length, struct whp_hda_regs *out)
{
    struct whp_hda_regs s = {0};
    if (!regs || !out || length < WHP_HDA_MMIO_BYTES)
        return WHP_HDA_REGS_BAD_INPUT;
    s.gcap = read16(regs);
    s.minor = regs[2u];
    s.major = regs[3u];
    if (s.major != 1u)
        return WHP_HDA_REGS_UNSUPPORTED_VERSION;
    s.output_streams = (uint8_t)((s.gcap >> 12) & 0xfu);
    s.input_streams = (uint8_t)((s.gcap >> 8) & 0xfu);
    s.bidir_streams = (uint8_t)((s.gcap >> 3) & 0x1fu);
    s.serial_data_outputs = (uint8_t)(((s.gcap >> 1) & 3u) + 1u);
    s.supports_64bit = (uint8_t)(s.gcap & 1u);
    s.gctl = read32(regs + 0x08u);
    s.statests = read16(regs + 0x0eu);
    /* Bits 6,5,4 are supported 256,16,2-entry ring sizes; the
     * lower two bits are selected size and MUST NOT be confused with
     * hardware capability. We never select or initialize a ring here.
     */
    s.corb_supported_sizes = (uint8_t)((regs[0x4eu] >> 4) & 7u);
    s.rirb_supported_sizes = (uint8_t)((regs[0x5eu] >> 4) & 7u);
    *out = s;
    return WHP_HDA_REGS_OK;
}
