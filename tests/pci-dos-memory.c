/* Synthetic BAR/window tests only; no PCI probing or DOS interrupts. */
#include "pci_dos_memory.h"
#include <stdio.h>
#include <string.h>

#define CHECK(x) do { ++checks; if (!(x)) { \
    fprintf(stderr, "DOS PCI memory test line %d: %s\n", __LINE__, #x); \
    return 1; } } while (0)

static void put32(uint8_t *p, unsigned int off, uint32_t value)
{
    p[off] = (uint8_t)value;
    p[off + 1u] = (uint8_t)(value >> 8);
    p[off + 2u] = (uint8_t)(value >> 16);
    p[off + 3u] = (uint8_t)(value >> 24);
}

int main(void)
{
    uint8_t cfg[64];
    struct whp_pci_dos_bar bar, before;
    unsigned int flags;
    int checks = 0;
    memset(cfg, 0, sizeof(cfg));
    memset(&bar, 0xa5, sizeof(bar));
    before = bar;
    CHECK(whp_pci_dos_read_bar(0, 64u, 0u, &bar) == -1);
    CHECK(whp_pci_dos_read_bar(cfg, 63u, 0u, &bar) == -1);
    CHECK(whp_pci_dos_read_bar(cfg, 64u, 0u, 0) == -1);
    CHECK(whp_pci_dos_read_bar(cfg, 64u, 0u, &bar) == 0);
    CHECK(memcmp(&bar, &before, sizeof(bar)) == 0);
    put32(cfg, 0x10u, UINT32_C(0xfeb00008));
    CHECK(whp_pci_dos_read_bar(cfg, 64u, 0u, &bar) == 1);
    CHECK(!bar.is_io && !bar.is_64 && bar.slots == 1u);
    CHECK(bar.base == UINT64_C(0xfeb00000));
    CHECK(whp_pci_dos_bar_region_of(&bar, 640u) == WHP_PCI_DOS_BAR_PHYSICAL_MAPPING_REQUIRED);
    put32(cfg, 0x14u, UINT32_C(0x00000301));
    CHECK(whp_pci_dos_read_bar(cfg, 64u, 1u, &bar) == 1);
    CHECK(bar.is_io && bar.base == UINT64_C(0x300));
    CHECK(whp_pci_dos_bar_region_of(&bar, 640u) == WHP_PCI_DOS_BAR_IO_PORT);
    put32(cfg, 0x14u, UINT32_C(0x00010001));
    CHECK(whp_pci_dos_read_bar(cfg, 64u, 1u, &bar) == 1);
    CHECK(whp_pci_dos_bar_region_of(&bar, 640u) == WHP_PCI_DOS_BAR_IO_OUTSIDE_16BIT);
    put32(cfg, 0x18u, UINT32_C(0x000a0000));
    CHECK(whp_pci_dos_read_bar(cfg, 64u, 2u, &bar) == 1);
    CHECK(whp_pci_dos_bar_region_of(&bar, 640u) == WHP_PCI_DOS_BAR_LEGACY_BASE);
    put32(cfg, 0x18u, UINT32_C(0x00090000));
    CHECK(whp_pci_dos_read_bar(cfg, 64u, 2u, &bar) == 1);
    CHECK(whp_pci_dos_bar_region_of(&bar, 640u) == WHP_PCI_DOS_BAR_CONVENTIONAL_BASE);
    put32(cfg, 0x1cu, UINT32_C(0x00000004));
    put32(cfg, 0x20u, UINT32_C(0x00000001));
    CHECK(whp_pci_dos_read_bar(cfg, 64u, 3u, &bar) == 1);
    CHECK(bar.is_64 && bar.slots == 2u && bar.base == UINT64_C(0x100000000));
    CHECK(whp_pci_dos_bar_region_of(&bar, 640u) == WHP_PCI_DOS_BAR_ABOVE_4G);
    put32(cfg, 0x24u, UINT32_C(0x80000004));
    before = bar;
    CHECK(whp_pci_dos_read_bar(cfg, 64u, 5u, &bar) == -1);
    CHECK(memcmp(&bar, &before, sizeof(bar)) == 0);
    put32(cfg, 0x10u, UINT32_C(0x00000002));
    CHECK(whp_pci_dos_read_bar(cfg, 64u, 0u, &bar) == -1);
    memset(cfg, 0, sizeof(cfg));
    cfg[0x0eu] = 0x01u;
    put32(cfg, 0x14u, UINT32_C(0x12345000));
    CHECK(whp_pci_dos_read_bar(cfg, 64u, 1u, &bar) == 1);
    CHECK(whp_pci_dos_read_bar(cfg, 64u, 2u, &bar) == -1);
    put32(cfg, 0x14u, UINT32_C(0x80000004));
    CHECK(whp_pci_dos_read_bar(cfg, 64u, 1u, &bar) == -1);
    put32(cfg, 0x10u, UINT32_C(0x80000004));
    CHECK(whp_pci_dos_read_bar(cfg, 64u, 0u, &bar) == 1);
    CHECK(bar.is_64 && bar.slots == 2u);
    flags = whp_pci_dos_window_flags(UINT64_C(0), UINT64_C(0x0fffff), 640u, 1);
    CHECK((flags & WHP_PCI_DOS_WINDOW_TOUCHES_CONVENTIONAL) != 0);
    CHECK((flags & WHP_PCI_DOS_WINDOW_DECODE_ENABLED) != 0);
    flags = whp_pci_dos_window_flags(UINT64_C(0x80000000), UINT64_C(0x80ffffff), 640u, 0);
    CHECK(flags == WHP_PCI_DOS_WINDOW_PRESENT);
    flags = whp_pci_dos_window_flags(UINT64_C(0x100000000), UINT64_C(0x100ffffff), 640u, 1);
    CHECK((flags & WHP_PCI_DOS_WINDOW_ABOVE_4G) != 0);
    flags = whp_pci_dos_window_flags(UINT64_C(0xfff00000), UINT64_C(0x1000fffff), 640u, 1);
    CHECK((flags & WHP_PCI_DOS_WINDOW_SPANS_4G) != 0);
    CHECK(whp_pci_dos_window_flags(UINT64_C(0x10000000), UINT64_C(0), 640u, 1) == 0u);
    puts("DOS PCI BAR/windows: synthetic memory safety checks passed");
    return 0;
}
