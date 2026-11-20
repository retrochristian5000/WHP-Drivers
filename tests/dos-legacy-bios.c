/* Synthetic BIOS/BDA data; no interrupts, port I/O or real hardware. */
#include "legacy_bios.h"
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { ++checks; if (!(condition)) { \
    fprintf(stderr, "DOS legacy BIOS test line %d: %s\n", \
            __LINE__, #condition); return 1; } } while (0)

int main(void)
{
    struct whp_dos_legacy_snapshot input;
    struct whp_dos_legacy_report output;
    struct whp_dos_legacy_report previous;
    uint8_t pci[64];
    int checks = 0;

    memset(&input, 0, sizeof(input));
    memset(&output, 0x5a, sizeof(output));
    previous = output;
    CHECK(!whp_dos_legacy_decode(0, &output));
    CHECK(!whp_dos_legacy_decode(&input, 0));
    CHECK(memcmp(&previous, &output, sizeof(output)) == 0);
    CHECK(whp_dos_legacy_decode(&input, &output));
    CHECK(output.bios_floppy_count == 0u);
    CHECK(output.bios_serial_count == 0u);
    CHECK(output.bios_parallel_count == 0u);
    CHECK(output.com_base[0] == 0u && output.lpt_base[0] == 0u);

    /* Two floppies, two serial, one parallel, math coprocessor, game. */
    input.equipment = (uint16_t)(1u | (1u << 6) |
                        (2u << 9) | (1u << 14) | (1u << 1) |
                        (1u << 12) | (2u << 4));
    input.com_base[0] = 0x03f8u;
    input.com_base[1] = 0x02f8u;
    input.lpt_base[0] = 0x0378u;
    CHECK(whp_dos_legacy_decode(&input, &output));
    CHECK(output.bios_floppy_count == 2u);
    CHECK(output.bios_serial_count == 2u);
    CHECK(output.bios_parallel_count == 1u);
    CHECK(output.bios_fpu_present == 1u);
    CHECK(output.bios_game_port_reported == 1u);
    CHECK(output.bios_video_mode_code == 2u);
    CHECK(output.com_base[0] == 0x03f8u);
    CHECK(output.com_base[1] == 0x02f8u);
    CHECK(output.lpt_base[0] == 0x0378u);
    CHECK(output.com_base[2] == 0u);
    /* BIOS and BDA can disagree. Preserve both, never synthesize ports. */
    input.equipment = 0u;
    CHECK(whp_dos_legacy_decode(&input, &output));
    CHECK(output.bios_serial_count == 0u && output.com_base[0] != 0u);
    CHECK(whp_dos_com_listed_count(&output) == 2u);
    CHECK(!whp_dos_com_base_conflict(&output, 0u));
    CHECK(!whp_dos_com_base_conflict(&output, 4u));
    CHECK(!whp_dos_com_base_conflict(0, 0u));
    CHECK(whp_dos_com_listed_count(0) == 0u);
    input.com_base[2] = input.com_base[0];
    CHECK(whp_dos_legacy_decode(&input, &output));
    CHECK(whp_dos_com_listed_count(&output) == 3u);
    CHECK(whp_dos_com_base_conflict(&output, 0u));
    CHECK(whp_dos_com_base_conflict(&output, 2u));
    CHECK(!whp_dos_com_base_conflict(&output, 1u));
    input.com_base[2] = 0u;

    memset(pci, 0, sizeof(pci));
    CHECK(!whp_dos_legacy_pci_isa_bridge(0, sizeof(pci)));
    CHECK(!whp_dos_legacy_pci_isa_bridge(pci, 63u));
    pci[0x0bu] = 0x06u;
    pci[0x0au] = 0x01u;
    CHECK(!whp_dos_legacy_pci_isa_bridge(pci, sizeof(pci)));
    pci[0] = 0x22u;
    pci[1] = 0x10u;
    CHECK(whp_dos_legacy_pci_isa_bridge(pci, sizeof(pci)));
    pci[0x0eu] = 1u;
    CHECK(!whp_dos_legacy_pci_isa_bridge(pci, sizeof(pci)));
    pci[0x0eu] = 0u;
    pci[0x0au] = 0x04u;
    CHECK(!whp_dos_legacy_pci_isa_bridge(pci, sizeof(pci)));
    pci[0x0au] = 0x01u;
    pci[0] = 0xffu;
    pci[1] = 0xffu;
    CHECK(!whp_dos_legacy_pci_isa_bridge(pci, sizeof(pci)));
    memset(pci, 0, sizeof(pci));
    CHECK(!whp_dos_pci_serial_candidate(0, 64u));
    CHECK(!whp_dos_pci_serial_candidate(pci, 63u));
    pci[0] = 0x34u;
    pci[1] = 0x12u;
    pci[0x0bu] = 0x07u;
    CHECK(whp_dos_pci_serial_candidate(pci, 64u));
    pci[0x0au] = 0x02u;
    CHECK(whp_dos_pci_serial_candidate(pci, 64u));
    pci[0x0au] = 0x03u;
    CHECK(!whp_dos_pci_serial_candidate(pci, 64u));
    pci[0x0au] = 0u;
    pci[0x0eu] = 1u;
    CHECK(!whp_dos_pci_serial_candidate(pci, 64u));
    pci[0x0eu] = 0u;
    pci[0] = 0xffu;
    pci[1] = 0xffu;
    CHECK(!whp_dos_pci_serial_candidate(pci, 64u));
    printf("DOS BIOS/ISA/serial: %d synthetic checks passed\n", checks);
    return 0;
}
