/* Brother MFC-290C fax capability policy: host/OS neutral, no I/O.
 * Documented functions are not implemented transport commands.
 */
#ifndef WHP_BROTHER_MFC290C_FAX_H
#define WHP_BROTHER_MFC290C_FAX_H

#include <stdint.h>

#define WHP_MFC290C_FAX_MODEM_MAX_BPS          14400u
#define WHP_MFC290C_FAX_MODEM_BASIC_VOIP_BPS    9600u

/* Operator-selected fax-line compatibility modes from the user guide.
 * These are nominal machine settings, not a host-controlled USB modem,
 * and do not describe the actual negotiated line rate.
 */
enum whp_mfc290c_fax_line_compatibility {
    WHP_MFC290C_FAX_LINE_NORMAL = 1,
    WHP_MFC290C_FAX_LINE_BASIC_VOIP
};

/* Read-only model metadata; returns 0 for invalid mode.
 * No physical device read, setting change, fax dialing or USB I/O.
 */
uint32_t whp_mfc290c_fax_line_nominal_bps(
    enum whp_mfc290c_fax_line_compatibility compatibility);
#define WHP_MFC290C_FAX_CHART1_MEMORY_PAGES     170u
#define WHP_MFC290C_FAX_BW_BROADCAST_LOCATIONS   90u

enum whp_mfc290c_fax_mode {
    WHP_MFC290C_FAX_STANDALONE_TX_MONO = 1,
    WHP_MFC290C_FAX_STANDALONE_TX_COLOR,
    WHP_MFC290C_FAX_STANDALONE_RX,
    WHP_MFC290C_FAX_PC_TX_MONO,
    WHP_MFC290C_FAX_PC_TX_COLOR,
    WHP_MFC290C_FAX_PC_RX
};

enum whp_mfc290c_fax_support {
    WHP_MFC290C_FAX_INVALID_MODE = 0,
    WHP_MFC290C_FAX_UNSUPPORTED,
    WHP_MFC290C_FAX_DOCUMENTED_ONLY
};

/* DOCUMENTED_ONLY never means that a host driver can send a fax.
 * The standalone fax modem is controlled by device firmware; no USB fax
 * submission or reception protocol has been authenticated.
 */
enum whp_mfc290c_fax_support whp_mfc290c_fax_mode_support(
    enum whp_mfc290c_fax_mode mode);

enum whp_mfc290c_pc_fax_image {
    WHP_MFC290C_PC_FAX_MONO = 1,
    WHP_MFC290C_PC_FAX_COLOR
};

struct whp_mfc290c_pc_fax_document {
    uint32_t page_count;
    enum whp_mfc290c_pc_fax_image image;
};

enum whp_mfc290c_pc_fax_policy {
    WHP_MFC290C_PC_FAX_BAD_DOCUMENT = 0,
    WHP_MFC290C_PC_FAX_UNSUPPORTED_IMAGE,
    WHP_MFC290C_PC_FAX_DOCUMENT_ACCEPTABLE
};

/* Document-only preflight: validates a prospective *sending* document.
 * Success makes no promise about USB payload encoding, destination dialing,
 * memory availability, actual transmission, or telephone-line readiness.
 * Do not impose the 170-page test-chart memory rating as a page-count limit.
 */
enum whp_mfc290c_pc_fax_policy whp_mfc290c_pc_fax_check(
    const struct whp_mfc290c_pc_fax_document *document);

#endif
