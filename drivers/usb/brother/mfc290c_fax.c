#include "mfc290c_fax.h"

#include <stddef.h>

enum whp_mfc290c_fax_support whp_mfc290c_fax_mode_support(
    enum whp_mfc290c_fax_mode mode)
{
    switch (mode) {
    case WHP_MFC290C_FAX_STANDALONE_TX_MONO:
    case WHP_MFC290C_FAX_STANDALONE_TX_COLOR:
    case WHP_MFC290C_FAX_STANDALONE_RX:
    case WHP_MFC290C_FAX_PC_TX_MONO:
        return WHP_MFC290C_FAX_DOCUMENTED_ONLY;
    case WHP_MFC290C_FAX_PC_TX_COLOR:
    case WHP_MFC290C_FAX_PC_RX:
        return WHP_MFC290C_FAX_UNSUPPORTED;
    default:
        return WHP_MFC290C_FAX_INVALID_MODE;
    }
}

enum whp_mfc290c_pc_fax_policy whp_mfc290c_pc_fax_check(
    const struct whp_mfc290c_pc_fax_document *document)
{
    if (document == NULL || document->page_count == 0u) {
        return WHP_MFC290C_PC_FAX_BAD_DOCUMENT;
    }
    if (document->image != WHP_MFC290C_PC_FAX_MONO) {
        return document->image == WHP_MFC290C_PC_FAX_COLOR ?
            WHP_MFC290C_PC_FAX_UNSUPPORTED_IMAGE :
            WHP_MFC290C_PC_FAX_BAD_DOCUMENT;
    }
    return WHP_MFC290C_PC_FAX_DOCUMENT_ACCEPTABLE;
}
