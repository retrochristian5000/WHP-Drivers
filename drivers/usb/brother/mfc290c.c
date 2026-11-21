#include "mfc290c.h"

static uint16_t read_le16(const uint8_t *bytes)
{
    return (uint16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8));
}

enum whp_mfc290c_usb_result whp_mfc290c_identify_usb(
    const uint8_t *descriptor, size_t length,
    struct whp_mfc290c_usb_identity *out)
{
    struct whp_mfc290c_usb_identity identity;

    if (descriptor == NULL || out == NULL ||
        length < WHP_MFC290C_USB_DEVICE_DESCRIPTOR_SIZE ||
        descriptor[0] != WHP_MFC290C_USB_DEVICE_DESCRIPTOR_SIZE ||
        descriptor[1] != 0x01u || descriptor[17] == 0u) {
        return WHP_MFC290C_USB_BAD_INPUT;
    }

    identity.vendor_id = read_le16(descriptor + 8u);
    identity.product_id = read_le16(descriptor + 10u);
    if (identity.vendor_id != WHP_MFC290C_USB_VENDOR_ID ||
        identity.product_id != WHP_MFC290C_USB_PRODUCT_ID) {
        return WHP_MFC290C_USB_OTHER_DEVICE;
    }

    identity.usb_bcd = read_le16(descriptor + 2u);
    identity.device_bcd = read_le16(descriptor + 12u);
    identity.configurations = descriptor[17];
    identity.serial_string_index = descriptor[16];
    *out = identity;
    return WHP_MFC290C_USB_MATCH;
}

unsigned int whp_mfc290c_documented_features(void)
{
    return WHP_MFC290C_PRINT | WHP_MFC290C_SCAN |
           WHP_MFC290C_STANDALONE_FAX | WHP_MFC290C_PC_FAX_SEND |
           WHP_MFC290C_PHOTO_CAPTURE | WHP_MFC290C_PC_MEDIA_ACCESS;
}
