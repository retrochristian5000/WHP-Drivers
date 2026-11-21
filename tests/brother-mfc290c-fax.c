/* Host-only fax capability policy tests; never access USB or the phone line. */
#include "mfc290c_fax.h"
#include "mfc290c.h"

#include <stdint.h>
#include <stdio.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "MFC-290C fax check failed at %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void)
{
    struct whp_mfc290c_pc_fax_document document;
    unsigned int features = whp_mfc290c_documented_features();

    CHECK(WHP_MFC290C_FAX_MODEM_MAX_BPS == 14400u);
    CHECK(WHP_MFC290C_FAX_CHART1_MEMORY_PAGES == 170u);
    CHECK(WHP_MFC290C_FAX_BW_BROADCAST_LOCATIONS == 90u);

    CHECK(whp_mfc290c_fax_mode_support(
          WHP_MFC290C_FAX_STANDALONE_TX_MONO) ==
          WHP_MFC290C_FAX_DOCUMENTED_ONLY);
    CHECK(whp_mfc290c_fax_mode_support(
          WHP_MFC290C_FAX_STANDALONE_TX_COLOR) ==
          WHP_MFC290C_FAX_DOCUMENTED_ONLY);
    CHECK(whp_mfc290c_fax_mode_support(
          WHP_MFC290C_FAX_STANDALONE_RX) ==
          WHP_MFC290C_FAX_DOCUMENTED_ONLY);
    CHECK(whp_mfc290c_fax_mode_support(
          WHP_MFC290C_FAX_PC_TX_MONO) ==
          WHP_MFC290C_FAX_DOCUMENTED_ONLY);
    CHECK(whp_mfc290c_fax_mode_support(
          WHP_MFC290C_FAX_PC_TX_COLOR) ==
          WHP_MFC290C_FAX_UNSUPPORTED);
    CHECK(whp_mfc290c_fax_mode_support(
          WHP_MFC290C_FAX_PC_RX) ==
          WHP_MFC290C_FAX_UNSUPPORTED);
    CHECK(whp_mfc290c_fax_mode_support((enum whp_mfc290c_fax_mode)0) ==
          WHP_MFC290C_FAX_INVALID_MODE);
    CHECK(whp_mfc290c_fax_mode_support((enum whp_mfc290c_fax_mode)999) ==
          WHP_MFC290C_FAX_INVALID_MODE);

    CHECK((features & WHP_MFC290C_STANDALONE_FAX) != 0u);
    CHECK((features & WHP_MFC290C_PC_FAX_SEND) != 0u);
    CHECK((features & WHP_MFC290C_PC_FAX_RECEIVE) == 0u);

    CHECK(whp_mfc290c_pc_fax_check(NULL) ==
          WHP_MFC290C_PC_FAX_BAD_DOCUMENT);

    document.page_count = 0u;
    document.image = WHP_MFC290C_PC_FAX_MONO;
    CHECK(whp_mfc290c_pc_fax_check(&document) ==
          WHP_MFC290C_PC_FAX_BAD_DOCUMENT);

    document.page_count = 1u;
    CHECK(whp_mfc290c_pc_fax_check(&document) ==
          WHP_MFC290C_PC_FAX_DOCUMENT_ACCEPTABLE);

    document.image = WHP_MFC290C_PC_FAX_COLOR;
    CHECK(whp_mfc290c_pc_fax_check(&document) ==
          WHP_MFC290C_PC_FAX_UNSUPPORTED_IMAGE);

    document.image = (enum whp_mfc290c_pc_fax_image)99;
    CHECK(whp_mfc290c_pc_fax_check(&document) ==
          WHP_MFC290C_PC_FAX_BAD_DOCUMENT);

    document.image = WHP_MFC290C_PC_FAX_MONO;
    document.page_count = 171u; /* Memory rating is not a job limit. */
    CHECK(whp_mfc290c_pc_fax_check(&document) ==
          WHP_MFC290C_PC_FAX_DOCUMENT_ACCEPTABLE);
    document.page_count = UINT32_MAX;
    CHECK(whp_mfc290c_pc_fax_check(&document) ==
          WHP_MFC290C_PC_FAX_DOCUMENT_ACCEPTABLE);

    puts("Brother MFC-290C: synthetic fax policy tests passed (no fax sent)");
    return 0;
}
