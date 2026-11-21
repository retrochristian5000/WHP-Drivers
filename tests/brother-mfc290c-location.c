/* Portable location-hint checks; synthetic observations, no USB access. */
#include "mfc290c_location.h"
#include "mfc290c.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "MFC-290C location check failed, line %d: %s\n", \
                __LINE__, #expr); \
        return 1; \
    } \
} while (0)

int main(void)
{
    uint8_t device[18] = {
        18u, 1u, 0u, 2u, 0u, 0u, 0u, 64u,
        0xf9u, 0x04u, 0xfdu, 0x01u, 0x10u, 0x01u,
        1u, 2u, 3u, 1u
    };
    uint32_t location = 0x12345678u;
    const char *invalid[] = {
        "", "0x", "0x0012000", "0x001200000", "00120000",
        " 0x00120000", "0x0012x000", "0x00120000 ",
        "0xFFFFFFFFF", "0x0012000\n", "0x00_20000"
    };
    struct whp_mfc290c_usb_identity identity;
    unsigned int i;

    CHECK(!whp_mfc290c_parse_location_hint(NULL, &location));
    CHECK(!whp_mfc290c_parse_location_hint("0x00120000", NULL));
    CHECK(location == 0x12345678u);
    CHECK(whp_mfc290c_parse_location_hint("0x00120000", &location));
    CHECK(location == 0x00120000u);
    CHECK(whp_mfc290c_parse_location_hint("0XFFFFFFFF", &location));
    CHECK(location == UINT32_MAX);
    CHECK(whp_mfc290c_parse_location_hint("0x00000000", &location));
    CHECK(location == 0u);
    for (i = 0u; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        CHECK(!whp_mfc290c_parse_location_hint(invalid[i], &location));
        CHECK(location == 0u);
    }

    /* Model identification must not depend on the Mac's topology hint. */
    CHECK(whp_mfc290c_identify_usb(device, sizeof(device), &identity) ==
          WHP_MFC290C_USB_MATCH);
    CHECK(whp_mfc290c_parse_location_hint("0x00130000", &location));
    CHECK(location != 0x00120000u);
    CHECK(whp_mfc290c_identify_usb(device, sizeof(device), &identity) ==
          WHP_MFC290C_USB_MATCH);
    CHECK(identity.vendor_id == WHP_MFC290C_USB_VENDOR_ID);
    CHECK(identity.product_id == WHP_MFC290C_USB_PRODUCT_ID);

    puts("Brother MFC-290C: USB location-hint regression passed");
    return 0;
}
