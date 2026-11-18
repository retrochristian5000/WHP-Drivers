#include "pci_dos_memory.h"

static uint32_t load32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int whp_pci_dos_read_bar(const uint8_t *cfg, size_t length,
                         unsigned int index, struct whp_pci_dos_bar *out)
{
    unsigned int max_bars;
    uint32_t raw, upper;
    struct whp_pci_dos_bar candidate;
    if (cfg == 0 || out == 0 || length < 64u)
        return -1;
    if ((cfg[0x0eu] & 0x7fu) == 0u)
        max_bars = 6u;
    else if ((cfg[0x0eu] & 0x7fu) == 1u)
        max_bars = 2u;
    else
        return -1;
    if (index >= max_bars || (cfg[0] == 0xffu && cfg[1] == 0xffu))
        return -1;
    raw = load32(cfg + 0x10u + 4u * index);
    if (raw == 0u || raw == UINT32_C(0xffffffff))
        return 0;
    candidate.index = (uint8_t)index;
    candidate.slots = 1u;
    candidate.is_io = (uint8_t)((raw & 1u) != 0u);
    candidate.is_64 = 0u;
    if (candidate.is_io) {
        candidate.base = (uint64_t)(raw & UINT32_C(0xfffffffc));
    } else {
        if ((raw & 0x06u) == 0x04u) {
            if (index + 1u >= max_bars)
                return -1;
            upper = load32(cfg + 0x14u + 4u * index);
            candidate.is_64 = 1u;
            candidate.slots = 2u;
            candidate.base = (((uint64_t)upper) << 32) |
                             (uint64_t)(raw & UINT32_C(0xfffffff0));
        } else if ((raw & 0x06u) == 0u) {
            candidate.base = (uint64_t)(raw & UINT32_C(0xfffffff0));
        } else {
            return -1;
        }
    }
    if (candidate.base == 0u)
        return 0; /* Zero BAR base is not an assigned address. */
    *out = candidate;
    return 1;
}

enum whp_pci_dos_bar_region whp_pci_dos_bar_region_of(
    const struct whp_pci_dos_bar *bar, unsigned int conventional_kb)
{
    if (bar->is_io)
        return bar->base > UINT64_C(0xffff) ?
            WHP_PCI_DOS_BAR_IO_OUTSIDE_16BIT : WHP_PCI_DOS_BAR_IO_PORT;
    if (bar->base >= UINT64_C(0x100000000))
        return WHP_PCI_DOS_BAR_ABOVE_4G;
    if (conventional_kb != 0u && conventional_kb <= 1024u &&
        bar->base < ((uint64_t)conventional_kb << 10))
        return WHP_PCI_DOS_BAR_CONVENTIONAL_BASE;
    if (bar->base < UINT64_C(0x100000))
        return WHP_PCI_DOS_BAR_LEGACY_BASE;
    return WHP_PCI_DOS_BAR_PHYSICAL_MAPPING_REQUIRED;
}

unsigned int whp_pci_dos_window_flags(uint64_t base, uint64_t limit,
                                      unsigned int conventional_kb,
                                      int pci_memory_decode_enabled)
{
    unsigned int flags;
    if (base > limit)
        return 0u;
    flags = WHP_PCI_DOS_WINDOW_PRESENT;
    if (pci_memory_decode_enabled)
        flags |= WHP_PCI_DOS_WINDOW_DECODE_ENABLED;
    if (base < UINT64_C(0x100000))
        flags |= WHP_PCI_DOS_WINDOW_TOUCHES_LOW_1M;
    if (conventional_kb != 0u && conventional_kb <= 1024u &&
        base < ((uint64_t)conventional_kb << 10))
        flags |= WHP_PCI_DOS_WINDOW_TOUCHES_CONVENTIONAL;
    if (base >= UINT64_C(0x100000000))
        flags |= WHP_PCI_DOS_WINDOW_ABOVE_4G;
    else if (limit >= UINT64_C(0x100000000))
        flags |= WHP_PCI_DOS_WINDOW_SPANS_4G;
    return flags;
}
