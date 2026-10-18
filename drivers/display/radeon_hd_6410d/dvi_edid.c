#include "dvi_edid.h"

/* Wholly passive EDID interpretation; no register maps or I2C APIs. */
enum whp_r6410d_edid_status
whp_r6410d_parse_edid_base(const unsigned char *base, size_t length,
                          struct whp_r6410d_edid_base *out)
{
    static const unsigned char header[8] =
        {0x00u, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0x00u};
    struct whp_r6410d_edid_base candidate = {0};
    unsigned int sum = 0;
    unsigned int i;
    unsigned int slot;

    if (out) {
        struct whp_r6410d_edid_base empty = {0};
        *out = empty;
    }
    if (!base || !out)
        return WHP_R6410D_EDID_INVALID_ARGUMENT;
    if (length != WHP_R6410D_EDID_BASE_BYTES)
        return WHP_R6410D_EDID_INVALID_LENGTH;

    for (i = 0; i < sizeof(header); ++i)
        if (base[i] != header[i])
            return WHP_R6410D_EDID_INVALID_HEADER;
    for (i = 0; i < WHP_R6410D_EDID_BASE_BYTES; ++i)
        sum += base[i];
    if ((sum & 0xffu) != 0)
        return WHP_R6410D_EDID_INVALID_CHECKSUM;
    /* Only established EDID 1.0..1.4 base-block rules are decoded. */
    if (base[18] != 1u || base[19] > 4u)
        return WHP_R6410D_EDID_UNSUPPORTED_VERSION;

    candidate.edid_version = base[18];
    candidate.edid_revision = base[19];
    candidate.digital_input = (unsigned char)((base[20] & 0x80u) != 0);
    candidate.advertised_extensions = base[126];

    for (slot = 0; slot < 4; ++slot) {
        const unsigned char *dtd = base + 54u + 18u * slot;
        unsigned int clock10k = (unsigned int)dtd[0] |
                                ((unsigned int)dtd[1] << 8);
        unsigned int hact, vact, hblank, vblank;

        if (!clock10k)
            continue; /* Monitor descriptor, not a detailed timing. */

        hact = (unsigned int)dtd[2] | ((unsigned int)(dtd[4] & 0xf0u) << 4);
        hblank = (unsigned int)dtd[3] | ((unsigned int)(dtd[4] & 0x0fu) << 8);
        vact = (unsigned int)dtd[5] | ((unsigned int)(dtd[7] & 0xf0u) << 4);
        vblank = (unsigned int)dtd[6] | ((unsigned int)(dtd[7] & 0x0fu) << 8);
        if (!hact || !vact || !hblank || !vblank)
            return WHP_R6410D_EDID_INVALID_TIMING;

        candidate.has_detailed_timing = 1u;
        candidate.timing_descriptor_index = (unsigned char)slot;
        candidate.h_active = (unsigned short)hact;
        candidate.v_active = (unsigned short)vact;
        candidate.h_blanking = (unsigned short)hblank;
        candidate.v_blanking = (unsigned short)vblank;
        candidate.pixel_clock_khz = clock10k * 10u;
        break; /* First detailed timing candidate; NO mode selection. */
    }

    *out = candidate;
    return WHP_R6410D_EDID_OK;
}
