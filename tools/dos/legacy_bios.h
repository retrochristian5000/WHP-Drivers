/* BIOS legacy-resource evidence for the WHPPROBE DOS inventory.
 * These values describe BIOS-reported PC-compatible interfaces. They do
 * not establish ISA expansion-card identity or physical ISA slot presence.
 */
#ifndef WHP_DOS_LEGACY_BIOS_H
#define WHP_DOS_LEGACY_BIOS_H

#include <stdint.h>

#define WHP_DOS_BDA_COM_PORTS 4u
#define WHP_DOS_BDA_LPT_PORTS 3u

struct whp_dos_legacy_snapshot {
    uint16_t equipment;
    uint16_t com_base[WHP_DOS_BDA_COM_PORTS];
    uint16_t lpt_base[WHP_DOS_BDA_LPT_PORTS];
};

struct whp_dos_legacy_report {
    uint16_t equipment;
    unsigned int bios_serial_count;
    unsigned int bios_parallel_count;
    unsigned int bios_floppy_count;
    unsigned int bios_fpu_present;
    unsigned int bios_game_port_reported;
    unsigned int bios_video_mode_code;
    uint16_t com_base[WHP_DOS_BDA_COM_PORTS];
    uint16_t lpt_base[WHP_DOS_BDA_LPT_PORTS];
};

/* Decode INT 11h equipment bits and a caller-supplied BIOS Data Area
 * snapshot. Return 0 for invalid pointers, leaving output unchanged.
 * This function is pure: no DOS/BIOS calls, port I/O or memory access.
 */
int whp_dos_legacy_decode(const struct whp_dos_legacy_snapshot *snapshot,
                          struct whp_dos_legacy_report *out);

/* Class 06:01 indicates a PCI-to-ISA bridge class, often an LPC bridge.
 * This does not prove the existence of external ISA slots.
 */
int whp_dos_legacy_pci_isa_bridge(const uint8_t *cfg,
                                  unsigned int length);

#endif
