#include "dos_version.h"

void whp_dos_version_classify(unsigned int reported_major,
                              unsigned int reported_minor,
                              int true_result_valid,
                              unsigned int true_major,
                              unsigned int true_minor,
                              struct whp_dos_version *out)
{
    if (out == 0)
        return;
    out->reported_major = (unsigned char)reported_major;
    out->reported_minor = (unsigned char)reported_minor;
    out->true_major = 0;
    out->true_minor = 0;
    out->true_known = 0;
    out->profile = WHP_DOS_PROFILE_UNVERIFIED;

    /* The int 21h/3306h contract applies to DOS 5+. Validate the caller's
     * sampled registers; no reported-version fallback authorizes APIs.
     */
    if (!true_result_valid || true_major < 5u || true_major > 99u ||
        true_minor >= 100u)
        return;

    out->true_major = (unsigned char)true_major;
    out->true_minor = (unsigned char)true_minor;
    out->true_known = 1;
    if (true_major == 6u && true_minor == 22u)
        out->profile = WHP_DOS_PROFILE_622;
    else if (true_major == 7u && true_minor == 0u)
        out->profile = WHP_DOS_PROFILE_700;
    else if (true_major == 7u && true_minor == 10u)
        out->profile = WHP_DOS_PROFILE_710;
    else if (true_major < 7u)
        out->profile = WHP_DOS_PROFILE_LEGACY;
    else
        out->profile = WHP_DOS_PROFILE_OTHER;
}
