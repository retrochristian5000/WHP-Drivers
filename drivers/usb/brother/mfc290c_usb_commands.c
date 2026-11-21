#include "mfc290c_usb_commands.h"

enum whp_mfc290c_usb_interface_hint whp_mfc290c_usb_hint_interface(
    uint8_t interface_class, uint8_t interface_subclass,
    uint8_t interface_protocol)
{
    if (interface_class == WHP_MFC290C_USB_VENDOR_CLASS) {
        return WHP_MFC290C_INTERFACE_VENDOR_CLASS;
    }
    if (interface_class != WHP_MFC290C_USB_PRINTER_CLASS ||
        interface_subclass != WHP_MFC290C_USB_PRINTER_SUBCLASS) {
        return WHP_MFC290C_INTERFACE_UNCLASSIFIED;
    }
    /* USB Printer Class 1.1 section 5.3. An FF protocol is not the
     * same thing as an FF interface class. Newer protocol extensions
     * are not interpreted without their own specification.
     */
    switch (interface_protocol) {
    case 0x01u:
        return WHP_MFC290C_INTERFACE_PRINTER_UNIDIRECTIONAL;
    case 0x02u:
        return WHP_MFC290C_INTERFACE_PRINTER_BIDIRECTIONAL;
    case 0x03u:
        return WHP_MFC290C_INTERFACE_PRINTER_IEEE1284_4;
    case 0xffu:
        return WHP_MFC290C_INTERFACE_PRINTER_VENDOR_PROTOCOL;
    default:
        return WHP_MFC290C_INTERFACE_UNCLASSIFIED;
    }
}

static uint16_t read_le16(const uint8_t *bytes)
{
    return (uint16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8));
}

int whp_mfc290c_usb_classify_setup(
    const uint8_t *setup, size_t length, uint8_t target_interface_class,
    struct whp_mfc290c_usb_setup_report *out)
{
    struct whp_mfc290c_usb_setup_report result;
    int printer;

    if (setup == NULL || out == NULL || length != WHP_MFC290C_USB_SETUP_LENGTH) {
        return 0;
    }
    result.bm_request_type = setup[0];
    result.b_request = setup[1];
    result.w_value = read_le16(setup + 2u);
    result.w_index = read_le16(setup + 4u);
    result.w_length = read_le16(setup + 6u);
    result.kind = WHP_MFC290C_SETUP_OPAQUE;
    result.name = WHP_MFC290C_USB_REQUEST_UNIDENTIFIED;
    printer = target_interface_class == WHP_MFC290C_USB_PRINTER_CLASS;

    if (result.bm_request_type == 0x80u &&
        result.b_request == 0x06u && result.w_length > 0u) {
        result.kind = WHP_MFC290C_SETUP_STANDARD_READ;
        result.name = WHP_MFC290C_USB_GET_DESCRIPTOR;
    } else if (result.bm_request_type == 0x80u &&
               result.b_request == 0x08u && result.w_length == 1u &&
               result.w_value == 0u && result.w_index == 0u) {
        result.kind = WHP_MFC290C_SETUP_STANDARD_READ;
        result.name = WHP_MFC290C_USB_GET_CONFIGURATION;
    } else if (result.bm_request_type == 0x81u &&
               result.b_request == 0x0au && result.w_length == 1u &&
               result.w_value == 0u) {
        result.kind = WHP_MFC290C_SETUP_STANDARD_READ;
        result.name = WHP_MFC290C_USB_GET_INTERFACE;
    } else if ((result.bm_request_type == 0x80u ||
                result.bm_request_type == 0x81u ||
                result.bm_request_type == 0x82u) &&
               result.b_request == 0x00u && result.w_length == 2u &&
               result.w_value == 0u) {
        result.kind = WHP_MFC290C_SETUP_STANDARD_READ;
        result.name = WHP_MFC290C_USB_GET_STATUS;
    } else if (printer && result.bm_request_type == 0xa1u &&
               result.b_request == 0u && result.w_length >= 2u) {
        result.kind = WHP_MFC290C_SETUP_PRINTER_READ_CANDIDATE;
        result.name = WHP_MFC290C_USB_PRINTER_GET_DEVICE_ID;
    } else if (printer && result.bm_request_type == 0xa1u &&
               result.b_request == 1u && result.w_value == 0u &&
               result.w_length == 1u) {
        result.kind = WHP_MFC290C_SETUP_PRINTER_READ_CANDIDATE;
        result.name = WHP_MFC290C_USB_PRINTER_GET_PORT_STATUS;
    } else if (printer &&
               (result.bm_request_type == 0x21u ||
                result.bm_request_type == 0x23u) &&
               result.b_request == 2u && result.w_value == 0u &&
               result.w_length == 0u) {
        /* The USB Printer Class 1.1 standard documents the legacy 0x23
         * recipient discrepancy. Both forms change printer state.
         */
        result.kind = WHP_MFC290C_SETUP_PRINTER_RESET;
        result.name = WHP_MFC290C_USB_PRINTER_SOFT_RESET;
    } else if ((result.bm_request_type & 0x80u) == 0u) {
        result.kind = WHP_MFC290C_SETUP_HOST_OUT_OR_MUTATING;
    }

    *out = result;
    return 1;
}

enum whp_mfc290c_usb_endpoint_hint whp_mfc290c_usb_hint_endpoint(
    uint8_t interface_class, uint8_t endpoint_address, uint8_t attributes)
{
    /* Endpoint 0 is always control. A printer-class data endpoint uses
     * bulk transfer type (bmAttributes bits 1:0 equal to 2).
     */
    if (interface_class != WHP_MFC290C_USB_PRINTER_CLASS ||
        (endpoint_address & 0x0fu) == 0u ||
        (endpoint_address & 0x70u) != 0u ||
        (attributes & 0x03u) != 0x02u) {
        return WHP_MFC290C_ENDPOINT_UNCLASSIFIED;
    }
    if ((endpoint_address & 0x80u) != 0u) {
        return WHP_MFC290C_ENDPOINT_PRINTER_BULK_IN_CANDIDATE;
    }
    return WHP_MFC290C_ENDPOINT_PRINTER_BULK_OUT_CANDIDATE;
}
