#ifndef WHP_DOS_VERSION_H
#define WHP_DOS_VERSION_H

/* Historical MS-DOS version profiles, not a license to use any API. */
enum whp_dos_api_profile {
    WHP_DOS_PROFILE_UNVERIFIED = 0,
    WHP_DOS_PROFILE_LEGACY,
    WHP_DOS_PROFILE_622,
    WHP_DOS_PROFILE_700,
    WHP_DOS_PROFILE_710,
    WHP_DOS_PROFILE_OTHER
};

struct whp_dos_version {
    unsigned char reported_major;
    unsigned char reported_minor;
    unsigned char true_major;
    unsigned char true_minor;
    unsigned char true_known;
    enum whp_dos_api_profile profile;
};

void whp_dos_version_classify(unsigned int reported_major,
                              unsigned int reported_minor,
                              int true_result_valid,
                              unsigned int true_major,
                              unsigned int true_minor,
                              struct whp_dos_version *out);

#endif
