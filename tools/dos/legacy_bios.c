#include "legacy_bios.h"

int whp_dos_legacy_decode(const struct whp_dos_legacy_snapshot *snapshot,
                          struct whp_dos_legacy_report *out)
{
    struct whp_dos_legacy_report report;
    unsigned int i;
    if (snapshot == 0 || out == 0)
        return 0;

    report.equipment = snapshot->equipment;
    report.bios_serial_count = (unsigned int)
        ((snapshot->equipment >> 9) & 7u);
    report.bios_parallel_count = (unsigned int)
        ((snapshot->equipment >> 14) & 3u);
    report.bios_floppy_count = (snapshot->equipment & 1u) ?
        (unsigned int)(((snapshot->equipment >> 6) & 3u) + 1u) : 0u;
    report.bios_fpu_present = (snapshot->equipment >> 1) & 1u;
    report.bios_game_port_reported = (snapshot->equipment >> 12) & 1u;
    report.bios_video_mode_code = (snapshot->equipment >> 4) & 3u;

    for (i = 0u; i < WHP_DOS_BDA_COM_PORTS; ++i)
        report.com_base[i] = snapshot->com_base[i];
    for (i = 0u; i < WHP_DOS_BDA_LPT_PORTS; ++i)
        report.lpt_base[i] = snapshot->lpt_base[i];

    *out = report;
    return 1;
}

int whp_dos_legacy_pci_isa_bridge(const uint8_t *cfg,
                                  unsigned int length)
{
    if (cfg == 0 || length < 64u ||
        (cfg[0] == 0xffu && cfg[1] == 0xffu) ||
        (cfg[0] == 0u && cfg[1] == 0u))
        return 0;
    return cfg[0x0bu] == 0x06u && cfg[0x0au] == 0x01u &&
           (cfg[0x0eu] & 0x7fu) == 0u;
}

/* Pure BDA evidence checks; no UART or PCI I/O. */
unsigned int whp_dos_com_listed_count(
    const struct whp_dos_legacy_report *report)
{
    unsigned int i, count = 0u;
    if (report == 0)
        return 0u;
    for (i = 0u; i < WHP_DOS_BDA_COM_PORTS; ++i)
        if (report->com_base[i] != 0u)
            ++count;
    return count;
}

int whp_dos_com_base_conflict(const struct whp_dos_legacy_report *report,
                              unsigned int index)
{
    unsigned int other;
    if (report == 0 || index >= WHP_DOS_BDA_COM_PORTS ||
        report->com_base[index] == 0u)
        return 0;
    for (other = 0u; other < WHP_DOS_BDA_COM_PORTS; ++other)
        if (other != index &&
            report->com_base[index] == report->com_base[other])
            return 1;
    return 0;
}

int whp_dos_pci_serial_candidate(const uint8_t *cfg, unsigned int length)
{
    if (cfg == 0 || length < 64u ||
        (cfg[0] == 0xffu && cfg[1] == 0xffu) ||
        (cfg[0] == 0u && cfg[1] == 0u))
        return 0;
    return cfg[0x0bu] == 0x07u &&
           (cfg[0x0au] == 0x00u || cfg[0x0au] == 0x02u) &&
           (cfg[0x0eu] & 0x7fu) == 0u;
}
