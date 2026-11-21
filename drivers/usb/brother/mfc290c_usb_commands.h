/* MFC-290C: offline USB SETUP and endpoint evidence classifier.
 * Does not construct, submit or authorize any USB request.
 */
#ifndef WHP_BROTHER_MFC290C_USB_COMMANDS_H
#define WHP_BROTHER_MFC290C_USB_COMMANDS_H

#include <stddef.h>
#include <stdint.h>

#define WHP_MFC290C_USB_SETUP_LENGTH 8u
#define WHP_MFC290C_USB_PRINTER_CLASS 0x07u
#define WHP_MFC290C_USB_PRINTER_SUBCLASS 0x01u
#define WHP_MFC290C_USB_VENDOR_CLASS 0xffu
#define WHP_MFC290C_USB_MASS_STORAGE_CLASS 0x08u
#define WHP_MFC290C_USB_SCSI_TRANSPARENT_SUBCLASS 0x06u
#define WHP_MFC290C_USB_BULK_ONLY_PROTOCOL 0x50u

/* A passive interpretation of USB *interface* descriptor bytes.
 * The result is not a driver binding or an authorization to issue a USB
 * printer request; class, subclass and protocol must be read together.
 */
enum whp_mfc290c_usb_interface_hint {
    WHP_MFC290C_INTERFACE_UNCLASSIFIED = 0,
    WHP_MFC290C_INTERFACE_PRINTER_UNIDIRECTIONAL,
    WHP_MFC290C_INTERFACE_PRINTER_BIDIRECTIONAL,
    WHP_MFC290C_INTERFACE_PRINTER_IEEE1284_4,
    WHP_MFC290C_INTERFACE_PRINTER_VENDOR_PROTOCOL,
    WHP_MFC290C_INTERFACE_VENDOR_CLASS,
    /* Standard MSC SCSI/Bulk-Only descriptor, not proven media access. */
    WHP_MFC290C_INTERFACE_STORAGE_SCSI_BULK_ONLY
};

enum whp_mfc290c_usb_interface_hint whp_mfc290c_usb_hint_interface(
    uint8_t interface_class, uint8_t interface_subclass,
    uint8_t interface_protocol);

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
