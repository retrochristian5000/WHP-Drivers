/* Synthetic EDID 1.3 fixtures, no DDC or GPU required.
 * The 1024x768 DTD is deliberately constructed, not device evidence.
 */
#include "dvi_edid.h"

static void seal_checksum(unsigned char *edid)
{
    unsigned int sum = 0, i;
    edid[127] = 0;
    for (i = 0; i < 127; ++i)
        sum += edid[i];
    edid[127] = (unsigned char)((0u - sum) & 0xffu);
}

static void fixture(unsigned char *edid)
{
    unsigned int i;
    unsigned char *dtd;
    for (i = 0; i < WHP_R6410D_EDID_BASE_BYTES; ++i)
        edid[i] = 0;
    for (i = 1; i < 7; ++i)
        edid[i] = 0xffu;
    edid[18] = 1u; edid[19] = 3u; /* EDID 1.3 */
    edid[20] = 0x80u; /* Digital video input, not a DVI connector ID. */
    edid[24] = 0x02u;
    dtd = edid + 54;
    dtd[0] = 0x64u; dtd[1] = 0x19u; /* 65,000 kHz */
    dtd[2] = 0x00u; dtd[3] = 0x40u; dtd[4] = 0x41u; /* 1024 + 320 */
    dtd[5] = 0x00u; dtd[6] = 0x26u; dtd[7] = 0x30u; /* 768 + 38 */
    seal_checksum(edid);
}

int main(void)
{
    unsigned char edid[WHP_R6410D_EDID_BASE_BYTES];
    struct whp_r6410d_edid_base out;
    unsigned int i;

    fixture(edid);
    if (whp_r6410d_parse_edid_base(edid, sizeof(edid), &out) != WHP_R6410D_EDID_OK ||
        !out.digital_input || !out.has_detailed_timing || out.edid_revision != 3u ||
        out.h_active != 1024u || out.h_blanking != 320u ||
        out.v_active != 768u || out.v_blanking != 38u ||
        out.pixel_clock_khz != 65000u || out.timing_descriptor_index != 0u)
        return 1;

    edid[20] = 0u; seal_checksum(edid);
    if (whp_r6410d_parse_edid_base(edid, sizeof(edid), &out) != WHP_R6410D_EDID_OK ||
        out.digital_input != 0u)
        return 2; /* Analog EDID is valid, but not a digital sink. */

    fixture(edid); edid[1] = 0u; seal_checksum(edid);
    if (whp_r6410d_parse_edid_base(edid, sizeof(edid), &out) != WHP_R6410D_EDID_INVALID_HEADER ||
        out.has_detailed_timing)
        return 3;

    fixture(edid); edid[20] ^= 0x01u; /* deliberately stale checksum */
    if (whp_r6410d_parse_edid_base(edid, sizeof(edid), &out) != WHP_R6410D_EDID_INVALID_CHECKSUM)
        return 4;

    fixture(edid);
    if (whp_r6410d_parse_edid_base(edid, sizeof(edid) - 1u, &out) != WHP_R6410D_EDID_INVALID_LENGTH ||
        whp_r6410d_parse_edid_base(edid, sizeof(edid) + 1u, &out) != WHP_R6410D_EDID_INVALID_LENGTH ||
        whp_r6410d_parse_edid_base(0, sizeof(edid), &out) != WHP_R6410D_EDID_INVALID_ARGUMENT ||
        whp_r6410d_parse_edid_base(edid, sizeof(edid), 0) != WHP_R6410D_EDID_INVALID_ARGUMENT)
        return 5;

    fixture(edid); edid[18] = 2u; seal_checksum(edid);
    if (whp_r6410d_parse_edid_base(edid, sizeof(edid), &out) != WHP_R6410D_EDID_UNSUPPORTED_VERSION)
        return 6;

    fixture(edid); edid[19] = 5u; seal_checksum(edid);
    if (whp_r6410d_parse_edid_base(edid, sizeof(edid), &out) != WHP_R6410D_EDID_UNSUPPORTED_VERSION)
        return 7;

    fixture(edid); edid[126] = 1u; seal_checksum(edid);
    if (whp_r6410d_parse_edid_base(edid, sizeof(edid), &out) != WHP_R6410D_EDID_OK ||
        out.advertised_extensions != 1u)
        return 8; /* Advertised only: extensions NOT read or validated. */

    fixture(edid); edid[54] = 0u; edid[55] = 0u; seal_checksum(edid);
    if (whp_r6410d_parse_edid_base(edid, sizeof(edid), &out) != WHP_R6410D_EDID_OK ||
        out.has_detailed_timing || out.pixel_clock_khz)
        return 9;

    fixture(edid); edid[57] = 0u; edid[58] &= 0xf0u; seal_checksum(edid);
    if (whp_r6410d_parse_edid_base(edid, sizeof(edid), &out) != WHP_R6410D_EDID_INVALID_TIMING)
        return 10;

    fixture(edid); edid[60] = 0u; edid[61] &= 0xf0u; seal_checksum(edid);
    if (whp_r6410d_parse_edid_base(edid, sizeof(edid), &out) != WHP_R6410D_EDID_INVALID_TIMING)
        return 11;

    /* A monitor descriptor may precede the first DTD. */
    fixture(edid);
    for (i = 0; i < 18; ++i) {
        unsigned char save = edid[54u + i];
        edid[72u + i] = save;
        edid[54u + i] = 0u;
    }
    seal_checksum(edid);
    if (whp_r6410d_parse_edid_base(edid, sizeof(edid), &out) != WHP_R6410D_EDID_OK ||
        out.timing_descriptor_index != 1u || out.h_active != 1024u)
        return 12;

    return 0;
}
