/* Read-only PCI BAR and bridge-window memory diagnostics for real-mode DOS.
 * BAR sizes, address ownership, physical memory map and DMA safety are unknown.
 */
#ifndef WHP_PCI95_DOS_MEMORY_H
#define WHP_PCI95_DOS_MEMORY_H

#include <stddef.h>
#include <stdint.h>

#define WHP_PCI_DOS_WINDOW_PRESENT              0x01u
#define WHP_PCI_DOS_WINDOW_DECODE_ENABLED       0x02u
#define WHP_PCI_DOS_WINDOW_TOUCHES_CONVENTIONAL 0x04u
#define WHP_PCI_DOS_WINDOW_TOUCHES_LOW_1M       0x08u
#define WHP_PCI_DOS_WINDOW_SPANS_4G             0x10u
#define WHP_PCI_DOS_WINDOW_ABOVE_4G             0x20u

struct whp_pci_dos_bar {
    uint64_t base;
    uint8_t index;
    uint8_t slots;
    uint8_t is_io;
    uint8_t is_64;
};

enum whp_pci_dos_bar_region {
    WHP_PCI_DOS_BAR_IO_PORT = 0,
    WHP_PCI_DOS_BAR_IO_OUTSIDE_16BIT,
    WHP_PCI_DOS_BAR_CONVENTIONAL_BASE,
    WHP_PCI_DOS_BAR_LEGACY_BASE,
    WHP_PCI_DOS_BAR_PHYSICAL_MAPPING_REQUIRED,
    WHP_PCI_DOS_BAR_ABOVE_4G
};

/* 1: assigned BAR candidate, 0: unassigned, -1: malformed/unknown.
 * Input must be type-0/type-1 standard header (64 bytes); no BAR sizing.
 * An invalid or absent result leaves output unchanged.
 */
int whp_pci_dos_read_bar(const uint8_t *cfg, size_t length,
                         unsigned int index, struct whp_pci_dos_bar *out);

enum whp_pci_dos_bar_region whp_pci_dos_bar_region_of(
    const struct whp_pci_dos_bar *bar, unsigned int conventional_kb);

/* A bit mask describing a supplied *inclusive* bridge memory window.
 * conventional_kb is the reported INT 12h bound, 0 if unavailable.
 * Neither a window nor its overlap proves an actual memory collision.
 */
unsigned int whp_pci_dos_window_flags(uint64_t base, uint64_t limit,
                                      unsigned int conventional_kb,
                                      int pci_memory_decode_enabled);

#endif
