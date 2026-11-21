/* Synthetic USB device-descriptor fixtures only: no physical USB I/O. */
#include "mfc290c.h"
#include <stdio.h>
#include <string.h>

#define CHECK(expression) do { \
    if (!(expression)) { \
        fprintf(stderr, "MFC-290C test failed at line %d: %s\n", \
                __LINE__, #expression); \
        return 1; \
    } \
} while (0)

int main(void)
{
    uint8_t descriptor[19] = {
        18u, 1u, 0x00u, 0x02u, 0u, 0u, 0u, 64u,
        0xf9u, 0x04u, 0xfdu, 0x01u, 0x10u, 0x01u,
        1u, 2u, 3u, 1u, 0x55u
    };
    struct whp_mfc290c_usb_identity identity;
    struct whp_mfc290c_usb_identity prior;
    unsigned int features = whp_mfc290c_documented_features();

    CHECK((features & WHP_MFC290C_PRINT) != 0u);
    CHECK((features & WHP_MFC290C_SCAN) != 0u);
    CHECK((features & WHP_MFC290C_STANDALONE_FAX) != 0u);
    CHECK((features & WHP_MFC290C_PC_FAX_SEND) != 0u);
    CHECK((features & WHP_MFC290C_PC_FAX_RECEIVE) == 0u);
    CHECK((features & WHP_MFC290C_PHOTO_CAPTURE) != 0u);
    CHECK((features & WHP_MFC290C_PC_MEDIA_ACCESS) != 0u);
    CHECK(WHP_MFC290C_PHOTO_CAPTURE != WHP_MFC290C_PRINT);
    CHECK(WHP_MFC290C_PHOTO_CAPTURE != WHP_MFC290C_SCAN);
    CHECK(WHP_MFC290C_PHOTO_CAPTURE != WHP_MFC290C_PC_MEDIA_ACCESS);
    CHECK((features & WHP_MFC290C_PC_FAX_RECEIVE) == 0u);

    memset(&identity, 0xa5, sizeof(identity));
    prior = identity;
    CHECK(whp_mfc290c_identify_usb(NULL, 18u, &identity) ==
          WHP_MFC290C_USB_BAD_INPUT);
    CHECK(whp_mfc290c_identify_usb(descriptor, 18u, NULL) ==
          WHP_MFC290C_USB_BAD_INPUT);
    CHECK(whp_mfc290c_identify_usb(descriptor, 17u, &identity) ==
          WHP_MFC290C_USB_BAD_INPUT);
    CHECK(memcmp(&identity, &prior, sizeof(identity)) == 0);

    CHECK(whp_mfc290c_identify_usb(descriptor, 18u, &identity) ==
          WHP_MFC290C_USB_MATCH);
    CHECK(identity.vendor_id == 0x04f9u && identity.product_id == 0x01fdu);
    CHECK(identity.usb_bcd == 0x0200u && identity.device_bcd == 0x0110u);
    CHECK(identity.configurations == 1u);
    CHECK(identity.serial_string_index == 3u);
    descriptor[16] = 0u; /* No USB serial string advertised. */
    CHECK(whp_mfc290c_identify_usb(descriptor, 18u, &identity) ==
          WHP_MFC290C_USB_MATCH);
    CHECK(identity.serial_string_index == 0u);
    descriptor[16] = 3u; /* Index, not an actual serial value. */
    CHECK(whp_mfc290c_identify_usb(descriptor, 18u, &identity) ==
          WHP_MFC290C_USB_MATCH);
    CHECK(identity.serial_string_index == 3u);
    CHECK(whp_mfc290c_identify_usb(descriptor, sizeof(descriptor), &identity) ==
          WHP_MFC290C_USB_MATCH); /* Trailing caller bytes are harmless. */

    prior = identity;
    descriptor[8] = 0xf8u;
    CHECK(whp_mfc290c_identify_usb(descriptor, 18u, &identity) ==
          WHP_MFC290C_USB_OTHER_DEVICE);
    descriptor[8] = 0xf9u;
    descriptor[10] = 0xfeu;
    CHECK(whp_mfc290c_identify_usb(descriptor, 18u, &identity) ==
          WHP_MFC290C_USB_OTHER_DEVICE); /* MFC-250C is NOT MFC-290C. */
    descriptor[10] = 0xfdu;
    CHECK(memcmp(&identity, &prior, sizeof(identity)) == 0);

    descriptor[0] = 17u;
    CHECK(whp_mfc290c_identify_usb(descriptor, 18u, &identity) ==
          WHP_MFC290C_USB_BAD_INPUT);
    descriptor[0] = 19u;
    CHECK(whp_mfc290c_identify_usb(descriptor, 19u, &identity) ==
          WHP_MFC290C_USB_BAD_INPUT);
    descriptor[0] = 18u;
    descriptor[1] = 2u; /* Configuration descriptor, not device descriptor. */
    CHECK(whp_mfc290c_identify_usb(descriptor, 18u, &identity) ==
          WHP_MFC290C_USB_BAD_INPUT);
    descriptor[1] = 1u;
    descriptor[17] = 0u;
    CHECK(whp_mfc290c_identify_usb(descriptor, 18u, &identity) ==
          WHP_MFC290C_USB_BAD_INPUT);
    CHECK(memcmp(&identity, &prior, sizeof(identity)) == 0);

    puts("Brother MFC-290C: synthetic USB identity and capability checks passed");
    return 0;
}
