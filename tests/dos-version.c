/* Host-only tests: synthetic DOS INT 21h register values; no interrupts. */
#include "dos_version.h"
#include <stdio.h>

#define CHECK(test) do { ++checks; if (!(test)) { \
    fprintf(stderr, "DOS version test failure at line %d: %s\n", \
            __LINE__, #test); return 1; } } while (0)

int main(void)
{
    struct whp_dos_version v;
    int checks = 0;

    whp_dos_version_classify(6u, 22u, 1, 6u, 22u, &v);
    CHECK(v.profile == WHP_DOS_PROFILE_622 && v.true_known);
    CHECK(v.reported_major == 6u && v.true_minor == 22u);
    whp_dos_version_classify(7u, 0u, 1, 7u, 0u, &v);
    CHECK(v.profile == WHP_DOS_PROFILE_700);
    whp_dos_version_classify(7u, 10u, 1, 7u, 10u, &v);
    CHECK(v.profile == WHP_DOS_PROFILE_710);
    whp_dos_version_classify(5u, 0u, 1, 7u, 10u, &v);
    CHECK(v.profile == WHP_DOS_PROFILE_710 && v.reported_major == 5u);
    whp_dos_version_classify(7u, 10u, 0, 0u, 0u, &v);
    CHECK(v.profile == WHP_DOS_PROFILE_UNVERIFIED && !v.true_known);
    whp_dos_version_classify(7u, 10u, 1, 4u, 22u, &v);
    CHECK(v.profile == WHP_DOS_PROFILE_UNVERIFIED && !v.true_known);
    whp_dos_version_classify(7u, 10u, 1, 7u, 100u, &v);
    CHECK(v.profile == WHP_DOS_PROFILE_UNVERIFIED && !v.true_known);
    whp_dos_version_classify(6u, 20u, 1, 6u, 20u, &v);
    CHECK(v.profile == WHP_DOS_PROFILE_LEGACY);
    whp_dos_version_classify(8u, 0u, 1, 8u, 0u, &v);
    CHECK(v.profile == WHP_DOS_PROFILE_OTHER);
    whp_dos_version_classify(6u, 22u, 1, 6u, 22u, 0);
    puts("DOS version profiles: synthetic checks passed");
    return checks < 9;
}
