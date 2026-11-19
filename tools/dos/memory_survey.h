/* Read-only, PC-compatible firmware memory-resource evidence.
 * NOT a RAM test, E820 map, DMA test or DOS memory allocator inspector.
 */
#ifndef WHP_DOS_MEMORY_SURVEY_H
#define WHP_DOS_MEMORY_SURVEY_H
#include <stdint.h>
struct whp_memory_firmware {
    uint16_t int12_kb, bda_kb, ebda_segment, int15_88_kb;
    uint8_t int15_88_valid;
};
struct whp_memory_survey {
    uint32_t conventional_end, ebda_base;
    uint16_t legacy_extended_kb;
    uint8_t conventional_valid, bda_valid, bda_mismatch;
    uint8_t ebda_candidate, ebda_below_conventional_end;
    uint8_t legacy_extended_valid;
};
enum whp_memory_bar_notice {
    WHP_MEM_BAR_NONE = 0,
    WHP_MEM_BAR_CONVENTIONAL,
    WHP_MEM_BAR_EBDA_CANDIDATE,
    WHP_MEM_BAR_LOW_GAP,
    WHP_MEM_BAR_LEGACY_APERTURE
};
int whp_memory_survey_decode(const struct whp_memory_firmware *firmware,
                              struct whp_memory_survey *out);
/* Only assigned MMIO BAR bases with enabled PCI memory decode.
 * A BAR base is NOT a known-size mapping or proof of address conflict.
 */
enum whp_memory_bar_notice whp_memory_bar_check(
    const struct whp_memory_survey *survey, uint64_t base,
    int is_io, int memory_decode_enabled);
#endif
