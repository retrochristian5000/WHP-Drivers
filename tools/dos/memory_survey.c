#include "memory_survey.h"
#include <string.h>
int whp_memory_survey_decode(const struct whp_memory_firmware *firmware,
                              struct whp_memory_survey *out)
{
    struct whp_memory_survey r;
    if (!firmware || !out)
        return 0;
    memset(&r, 0, sizeof(r));
    if (firmware->int12_kb >= 64u && firmware->int12_kb <= 640u) {
        r.conventional_valid = 1u;
        r.conventional_end = (uint32_t)firmware->int12_kb * 1024u;
    }
    if (firmware->bda_kb >= 64u && firmware->bda_kb <= 640u) {
        r.bda_valid = 1u;
        if (r.conventional_valid &&
            firmware->bda_kb != firmware->int12_kb)
            r.bda_mismatch = 1u;
    }
    /* BDA 0040:000E is also LPT4 on some systems: EBDA is speculative.
     * Never dereference that address, even when it appears plausible.
     */
    if (firmware->ebda_segment >= 0x8000u &&
        firmware->ebda_segment < 0xa000u) {
        r.ebda_candidate = 1u;
        r.ebda_base = (uint32_t)firmware->ebda_segment << 4;
        if (r.conventional_valid && r.ebda_base < r.conventional_end)
            r.ebda_below_conventional_end = 1u;
    }
    if (firmware->int15_88_valid) {
        r.legacy_extended_valid = 1u;
        r.legacy_extended_kb = firmware->int15_88_kb;
    }
    *out = r;
    return 1;
}
enum whp_memory_bar_notice whp_memory_bar_check(
    const struct whp_memory_survey *survey, uint64_t base,
    int is_io, int memory_decode_enabled)
{
    if (!survey || is_io || !memory_decode_enabled ||
        base == UINT64_C(0) || base >= UINT64_C(0x100000) ||
        !survey->conventional_valid)
        return WHP_MEM_BAR_NONE;
    if (base < (uint64_t)survey->conventional_end)
        return WHP_MEM_BAR_CONVENTIONAL;
    if (base < UINT64_C(0xa0000)) {
        if (survey->ebda_candidate && base >= (uint64_t)survey->ebda_base)
            return WHP_MEM_BAR_EBDA_CANDIDATE;
        return WHP_MEM_BAR_LOW_GAP;
    }
    /* VGA and option ROM/UMB apertures can be legitimate mappings. */
    return WHP_MEM_BAR_LEGACY_APERTURE;
}
