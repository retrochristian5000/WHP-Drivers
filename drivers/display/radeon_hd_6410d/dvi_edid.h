/* WHP Radeon HD 6410D: read-only EDID 1.x base-block decoder.
 * EDID identifies a display and possible timings; it does NOT identify
 * the physical DVI-I/DVI-D connector, prove TMDS availability, or select
 * any video mode. No DDC/I2C transactions or GPU register accesses.
 *
 * Reference: Linux DRM drm_edid.h (digital input and DTD field encoding).
 * https://gbmc.googlesource.com/linux/+/5e049663f678938dd56812ba870a471060f8ce28/include/drm/drm_edid.h
 */
#ifndef WHP_R6410D_DVI_EDID_H
#define WHP_R6410D_DVI_EDID_H

#include <stddef.h>

#define WHP_R6410D_EDID_BASE_BYTES 128u

enum whp_r6410d_edid_status {
    WHP_R6410D_EDID_OK = 0,
    WHP_R6410D_EDID_INVALID_ARGUMENT = -1,
    WHP_R6410D_EDID_INVALID_LENGTH = -2,
    WHP_R6410D_EDID_INVALID_HEADER = -3,
    WHP_R6410D_EDID_INVALID_CHECKSUM = -4,
    WHP_R6410D_EDID_UNSUPPORTED_VERSION = -5,
    WHP_R6410D_EDID_INVALID_TIMING = -6
};

/* No platform-specific packing, pointers, or compiler pragmas in ABI data.
 * "first timing" is a candidate, not an approved framebuffer/video mode.
 */
struct whp_r6410d_edid_base {
    unsigned char edid_version;
    unsigned char edid_revision;
    unsigned char digital_input;
    unsigned char advertised_extensions; /* NOT validated or decoded */
    unsigned char has_detailed_timing;
    unsigned char timing_descriptor_index; /* 0..3, only if present */
    unsigned short h_active;
    unsigned short v_active;
    unsigned short h_blanking;
    unsigned short v_blanking;
    unsigned int pixel_clock_khz;
};

/* Requires EXACTLY the 128-byte EDID 1.x base block, validated before
 * exposing metadata. On any failure *out is zeroed, if non-NULL.
 * A successful parse never means that DVI hardware is connected or usable.
 */
enum whp_r6410d_edid_status
whp_r6410d_parse_edid_base(const unsigned char *base, size_t length,
                          struct whp_r6410d_edid_base *out);

#endif
