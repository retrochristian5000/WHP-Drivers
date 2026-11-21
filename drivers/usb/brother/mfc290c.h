/* Brother MFC-290C: OS-neutral USB device identification only.
 * No device transfers, interface claims, printing, scanning or fax commands.
 */
#ifndef WHP_BROTHER_MFC290C_H
#define WHP_BROTHER_MFC290C_H

#include <stddef.h>
#include <stdint.h>

#define WHP_MFC290C_USB_VENDOR_ID  0x04f9u
#define WHP_MFC290C_USB_PRODUCT_ID 0x01fdu
#define WHP_MFC290C_USB_DEVICE_DESCRIPTOR_SIZE 18u

enum whp_mfc290c_usb_result {
    WHP_MFC290C_USB_BAD_INPUT = 0,
    WHP_MFC290C_USB_OTHER_DEVICE,
    WHP_MFC290C_USB_MATCH
};

struct whp_mfc290c_usb_identity {
    uint16_t vendor_id;
    uint16_t product_id;
    uint16_t usb_bcd;
    uint16_t device_bcd;
    uint8_t configurations;
    /* USB device descriptor byte 16: index, never the serial string. */
    uint8_t serial_string_index;
};

enum whp_mfc290c_documented_feature {
    WHP_MFC290C_PRINT = 1u << 0,
    WHP_MFC290C_SCAN = 1u << 1,
    WHP_MFC290C_STANDALONE_FAX = 1u << 2,
    WHP_MFC290C_PC_FAX_SEND = 1u << 3,
    WHP_MFC290C_PC_FAX_RECEIVE = 1u << 4,
    /* PhotoCapture Center from memory card/USB media: not a host USB
     * mass-storage binding or proof of device firmware commands.
     */
    WHP_MFC290C_PHOTO_CAPTURE = 1u << 5
};

/* Parse a caller-supplied raw USB *device* descriptor (not a PCI header).
 * Returns MATCH only for an intact 04f9:01fd descriptor. On any other result
 * the output is unchanged. A match does not establish any USB interfaces,
 * endpoints, active configuration, negotiated speed or working driver.
 */
enum whp_mfc290c_usb_result whp_mfc290c_identify_usb(
    const uint8_t *descriptor, size_t length,
    struct whp_mfc290c_usb_identity *out);

/* Features documented for this model, not implemented host-side operations.
 * PhotoCapture is a local media feature, not an authenticated USB host
 * transport or a control-panel button event exposed to the computer.
 * In particular, standalone fax reception does not imply PC-FAX reception.
 */
unsigned int whp_mfc290c_documented_features(void);

#endif
