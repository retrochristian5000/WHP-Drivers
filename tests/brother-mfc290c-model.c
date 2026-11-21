/* Synthetic CUPS queue metadata, not evidence of a physical printer. */
#include "mfc290c_model_match.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "MFC-290C CUPS label test failed at line %d: %s\n", \
                __LINE__, #expr); \
        return 1; \
    } \
} while (0)

int main(void)
{
    char oversized[258];

    CHECK(!whp_mfc290c_cups_model_candidate(NULL));
    CHECK(!whp_mfc290c_cups_model_candidate(""));
    CHECK(whp_mfc290c_cups_model_candidate("Brother MFC-290C CUPS"));
    CHECK(whp_mfc290c_cups_model_candidate("BROTHER MFC290C Printer"));
    CHECK(whp_mfc290c_cups_model_candidate("Brother MFC-290C"));
    CHECK(whp_mfc290c_cups_model_candidate("Brother MFC-290C series"));
    CHECK(!whp_mfc290c_cups_model_candidate("Brother MFC-290CW"));
    CHECK(!whp_mfc290c_cups_model_candidate("Brother MFC-290CXL"));
    CHECK(!whp_mfc290c_cups_model_candidate("Brother MFC-290"));
    CHECK(!whp_mfc290c_cups_model_candidate("Brother MFC-250C"));
    CHECK(!whp_mfc290c_cups_model_candidate("Generic MFC-290C"));
    CHECK(!whp_mfc290c_cups_model_candidate("Brotherish MFC-290C"));
    CHECK(!whp_mfc290c_cups_model_candidate("Brother MFC-290C0"));

    memset(oversized, 'X', sizeof(oversized));
    oversized[sizeof(oversized) - 1u] = '\0';
    CHECK(!whp_mfc290c_cups_model_candidate(oversized));

    puts("Brother MFC-290C: synthetic CUPS model-label tests passed");
    return 0;
}
