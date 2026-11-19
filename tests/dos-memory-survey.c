/* Host-only firmware and BAR base tests; no physical memory access. */
#include "memory_survey.h"
#include <stdio.h>
#include <string.h>
#define CHECK(expr) do { ++checks; if (!(expr)) { \
    fprintf(stderr, "DOS memory survey failure at line %d: %s\n", \
            __LINE__, #expr); return 1; } } while (0)
int main(void)
{
    struct whp_memory_firmware f;
    struct whp_memory_survey r;
    int checks = 0;
    memset(&f, 0, sizeof(f));
    memset(&r, 0xa5, sizeof(r));
    CHECK(!whp_memory_survey_decode(0, &r));
    CHECK(!whp_memory_survey_decode(&f, 0));
    CHECK(r.conventional_end == UINT32_C(0xa5a5a5a5));
    f.int12_kb = 638u; f.bda_kb = 638u; f.ebda_segment = 0x9fc0u;
    f.int15_88_kb = 15360u; f.int15_88_valid = 1u;
    CHECK(whp_memory_survey_decode(&f, &r));
    CHECK(r.conventional_valid && r.conventional_end == 638u * 1024u);
    CHECK(r.bda_valid && !r.bda_mismatch);
    CHECK(r.ebda_candidate && r.ebda_base == UINT32_C(0x9fc00));
    CHECK(!r.ebda_below_conventional_end);
    CHECK(r.legacy_extended_valid && r.legacy_extended_kb == 15360u);
    CHECK(whp_memory_bar_check(&r, UINT64_C(0x90000), 0, 1) ==
          WHP_MEM_BAR_CONVENTIONAL);
    CHECK(whp_memory_bar_check(&r, UINT64_C(0x9fc00), 0, 1) ==
          WHP_MEM_BAR_EBDA_CANDIDATE);
    CHECK(whp_memory_bar_check(&r, UINT64_C(0x9fb00), 0, 1) ==
          WHP_MEM_BAR_LOW_GAP);
    CHECK(whp_memory_bar_check(&r, UINT64_C(0xa0000), 0, 1) ==
          WHP_MEM_BAR_LEGACY_APERTURE);
    CHECK(whp_memory_bar_check(&r, UINT64_C(0xc0000), 0, 1) ==
          WHP_MEM_BAR_LEGACY_APERTURE);
    CHECK(whp_memory_bar_check(&r, UINT64_C(0x90000), 0, 0) ==
          WHP_MEM_BAR_NONE);
    CHECK(whp_memory_bar_check(&r, UINT64_C(0x90000), 1, 1) ==
          WHP_MEM_BAR_NONE);
    CHECK(whp_memory_bar_check(&r, UINT64_C(0x100000), 0, 1) ==
          WHP_MEM_BAR_NONE);
    CHECK(whp_memory_bar_check(&r, UINT64_C(0x100000000), 0, 1) ==
          WHP_MEM_BAR_NONE);
    f.bda_kb = 640u;
    CHECK(whp_memory_survey_decode(&f, &r) && r.bda_mismatch);
    f.ebda_segment = 0x8000u;
    CHECK(whp_memory_survey_decode(&f, &r) &&
          r.ebda_below_conventional_end);
    f.ebda_segment = 0x3f8u; /* LPT4, not EBDA. */
    CHECK(whp_memory_survey_decode(&f, &r) && !r.ebda_candidate);
    f.int12_kb = 1024u;
    CHECK(whp_memory_survey_decode(&f, &r) && !r.conventional_valid);
    CHECK(whp_memory_bar_check(&r, UINT64_C(0x90000), 0, 1) ==
          WHP_MEM_BAR_NONE);
    f.int15_88_valid = 0u;
    CHECK(whp_memory_survey_decode(&f, &r) &&
          !r.legacy_extended_valid);
    printf("DOS memory resource survey: %d tests passed\n", checks);
    return 0;
}
