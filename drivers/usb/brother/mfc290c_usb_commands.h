/* MFC-290C: offline USB SETUP and endpoint evidence classifier.
 * Does not construct, submit or authorize any USB request.
 */
#ifndef WHP_BROTHER_MFC290C_USB_COMMANDS_H
#define WHP_BROTHER_MFC290C_USB_COMMANDS_H

#include <stddef.h>
#include <stdint.h>

#define WHP_MFC290C_USB_SETUP_LENGTH 8u
#define WHP_MFC290C_USB_PRINTER_CLASS 0x07u

enum whp_mfc290c_usb_setup_kind {
    WHP_MFC290C_SETUP_OPAQUE = 0,
    WHP_MFC290C_SETUP_STANDARD_READ,
    WHP_MFC290C_SETUP_PRINTER_READ_CANDIDATE,
    WHP_MFC290C_SETUP_PRINTER_RESET,
    WHP_MFC290C_SETUP_HOST_OUT_OR_MUTATING
};

enum whp_mfc290c_usb_request_name {
    WHP_MFC290C_USB_REQUEST_UNIDENTIFIED = 0,
    WHP_MFC290C_USB_GET_DESCRIPTOR,
    WHP_MFC290C_USB_GET_CONFIGURATION,
    WHP_MFC290C_USB_GET_INTERFACE,
    WHP_MFC290C_USB_GET_STATUS,
    WHP_MFC290C_USB_PRINTER_GET_DEVICE_ID,
    WHP_MFC290C_USB_PRINTER_GET_PORT_STATUS,
    WHP_MFC290C_USB_PRINTER_SOFT_RESET
};

struct whp_mfc290c_usb_setup_report {
    uint8_t bm_request_type;
    uint8_t b_request;
    uint16_t w_value;
    uint16_t w_index;
    uint16_t w_length;
    enum whp_mfc290c_usb_setup_kind kind;
    enum whp_mfc290c_usb_request_name name;
};

/* Caller supplies a captured eight-byte USB SETUP packet and the class of
 * its target interface, if independently verified from a raw descriptor.
 * Pass 0xff when unknown. No MFC-290C interface class is yet verified.
 * The output is unchanged on invalid input. Returns 1 for parsed input.
 * A *_CANDIDATE label confirms only standard request syntax, not that the
 * particular Brother device accepts the request or uses it for printing.
 */
int whp_mfc290c_usb_classify_setup(
    const uint8_t *setup, size_t length, uint8_t target_interface_class,
    struct whp_mfc290c_usb_setup_report *out);

enum whp_mfc290c_usb_endpoint_hint {
    WHP_MFC290C_ENDPOINT_UNCLASSIFIED = 0,
    WHP_MFC290C_ENDPOINT_PRINTER_BULK_OUT_CANDIDATE,
    WHP_MFC290C_ENDPOINT_PRINTER_BULK_IN_CANDIDATE
};

/* Endpoint addresses and attributes must come from an actual configuration
 * descriptor, not a Windows MI_XX ID, USB location or assumed port number.
 * Scanner/fax stream roles stay unclassified until real protocol evidence.
 */
enum whp_mfc290c_usb_endpoint_hint whp_mfc290c_usb_hint_endpoint(
    uint8_t interface_class, uint8_t endpoint_address, uint8_t attributes);

#endif
