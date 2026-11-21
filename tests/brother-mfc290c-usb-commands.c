/* Synthetic USB captures, not packets transmitted to a Brother device. */
#include "mfc290c_usb_commands.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "USB commands line %d failed: %s\n", __LINE__, #expr); \
        return 1; \
    } \
} while (0)

static int expect(const uint8_t *packet, uint8_t interface_class,
                  enum whp_mfc290c_usb_setup_kind kind,
                  enum whp_mfc290c_usb_request_name name)
{
    struct whp_mfc290c_usb_setup_report report;
    CHECK(whp_mfc290c_usb_classify_setup(packet, 8u, interface_class,
                                         &report) == 1);
    CHECK(report.kind == kind);
    CHECK(report.name == name);
    CHECK(report.bm_request_type == packet[0]);
    CHECK(report.b_request == packet[1]);
    CHECK(report.w_value == (uint16_t)(packet[2] | (packet[3] << 8)));
    CHECK(report.w_index == (uint16_t)(packet[4] | (packet[5] << 8)));
    CHECK(report.w_length == (uint16_t)(packet[6] | (packet[7] << 8)));
    return 0;
}

int main(void)
{
    /* Standard USB read-only enumeration setup records. */
    const uint8_t descriptor[8] = {0x80u, 0x06u, 0x00u, 0x01u,
                                   0x00u, 0x00u, 0x12u, 0x00u};
    const uint8_t configuration[8] = {0x80u, 0x08u, 0x00u, 0x00u,
                                      0x00u, 0x00u, 0x01u, 0x00u};
    const uint8_t get_interface[8] = {0x81u, 0x0au, 0x00u, 0x00u,
                                      0x01u, 0x00u, 0x01u, 0x00u};
    const uint8_t get_status[8] = {0x82u, 0x00u, 0x00u, 0x00u,
                                   0x81u, 0x00u, 0x02u, 0x00u};
    /* Printer class only if the actual interface descriptor says 0x07. */
    const uint8_t printer_id[8] = {0xa1u, 0x00u, 0x00u, 0x00u,
                                   0x00u, 0x00u, 0xffu, 0x00u};
    const uint8_t printer_status[8] = {0xa1u, 0x01u, 0x00u, 0x00u,
                                       0x00u, 0x00u, 0x01u, 0x00u};
    const uint8_t printer_reset[8] = {0x21u, 0x02u, 0x00u, 0x00u,
                                      0x00u, 0x00u, 0x00u, 0x00u};
    const uint8_t legacy_reset[8] = {0x23u, 0x02u, 0x00u, 0x00u,
                                     0x00u, 0x00u, 0x00u, 0x00u};
    const uint8_t vendor_in[8] = {0xc0u, 0x00u, 0x00u, 0x00u,
                                  0x00u, 0x00u, 0x01u, 0x00u};
    const uint8_t vendor_out[8] = {0x40u, 0x00u, 0x00u, 0x00u,
                                   0x00u, 0x00u, 0x01u, 0x00u};
    const uint8_t malformed_status[8] = {0xa1u, 0x01u, 0x01u, 0x00u,
                                         0x00u, 0x00u, 0x01u, 0x00u};
    struct whp_mfc290c_usb_setup_report report;
    struct whp_mfc290c_usb_setup_report before;

    CHECK(expect(descriptor, 0xffu, WHP_MFC290C_SETUP_STANDARD_READ,
                 WHP_MFC290C_USB_GET_DESCRIPTOR) == 0);
    CHECK(expect(configuration, 0xffu, WHP_MFC290C_SETUP_STANDARD_READ,
                 WHP_MFC290C_USB_GET_CONFIGURATION) == 0);
    CHECK(expect(get_interface, 0xffu, WHP_MFC290C_SETUP_STANDARD_READ,
                 WHP_MFC290C_USB_GET_INTERFACE) == 0);
    CHECK(expect(get_status, 0xffu, WHP_MFC290C_SETUP_STANDARD_READ,
                 WHP_MFC290C_USB_GET_STATUS) == 0);
    CHECK(expect(printer_id, 0x07u,
                 WHP_MFC290C_SETUP_PRINTER_READ_CANDIDATE,
                 WHP_MFC290C_USB_PRINTER_GET_DEVICE_ID) == 0);
    CHECK(expect(printer_status, 0x07u,
                 WHP_MFC290C_SETUP_PRINTER_READ_CANDIDATE,
                 WHP_MFC290C_USB_PRINTER_GET_PORT_STATUS) == 0);
    CHECK(expect(printer_reset, 0x07u, WHP_MFC290C_SETUP_PRINTER_RESET,
                 WHP_MFC290C_USB_PRINTER_SOFT_RESET) == 0);
    CHECK(expect(legacy_reset, 0x07u, WHP_MFC290C_SETUP_PRINTER_RESET,
                 WHP_MFC290C_USB_PRINTER_SOFT_RESET) == 0);
    CHECK(expect(printer_id, 0xffu, WHP_MFC290C_SETUP_OPAQUE,
                 WHP_MFC290C_USB_REQUEST_UNIDENTIFIED) == 0);
    CHECK(expect(printer_reset, 0xffu,
                 WHP_MFC290C_SETUP_HOST_OUT_OR_MUTATING,
                 WHP_MFC290C_USB_REQUEST_UNIDENTIFIED) == 0);
    CHECK(expect(vendor_in, 0x07u, WHP_MFC290C_SETUP_OPAQUE,
                 WHP_MFC290C_USB_REQUEST_UNIDENTIFIED) == 0);
    CHECK(expect(vendor_out, 0x07u,
                 WHP_MFC290C_SETUP_HOST_OUT_OR_MUTATING,
                 WHP_MFC290C_USB_REQUEST_UNIDENTIFIED) == 0);
    CHECK(expect(malformed_status, 0x07u, WHP_MFC290C_SETUP_OPAQUE,
                 WHP_MFC290C_USB_REQUEST_UNIDENTIFIED) == 0);

    memset(&report, 0xa5, sizeof(report));
    before = report;
    CHECK(!whp_mfc290c_usb_classify_setup(NULL, 8u, 0x07u, &report));
    CHECK(!whp_mfc290c_usb_classify_setup(descriptor, 7u, 0x07u, &report));
    CHECK(!whp_mfc290c_usb_classify_setup(descriptor, 9u, 0x07u, &report));
    CHECK(!whp_mfc290c_usb_classify_setup(descriptor, 8u, 0x07u, NULL));
    CHECK(memcmp(&report, &before, sizeof(report)) == 0);

    /* Do not conflate an FF protocol with an FF interface class. */
    CHECK(whp_mfc290c_usb_hint_interface(0x07u, 0x01u, 0x01u) ==
          WHP_MFC290C_INTERFACE_PRINTER_UNIDIRECTIONAL);
    CHECK(whp_mfc290c_usb_hint_interface(0x07u, 0x01u, 0x02u) ==
          WHP_MFC290C_INTERFACE_PRINTER_BIDIRECTIONAL);
    CHECK(whp_mfc290c_usb_hint_interface(0x07u, 0x01u, 0x03u) ==
          WHP_MFC290C_INTERFACE_PRINTER_IEEE1284_4);
    CHECK(whp_mfc290c_usb_hint_interface(0x07u, 0x01u, 0xffu) ==
          WHP_MFC290C_INTERFACE_PRINTER_VENDOR_PROTOCOL);
    /* Hardware-observed FF/FF/FF is still a vendor *class*,
     * not printer class 07/01/FF or evidence of a vendor request.
     */
    CHECK(whp_mfc290c_usb_hint_interface(0xffu, 0xffu, 0xffu) ==
          WHP_MFC290C_INTERFACE_VENDOR_CLASS);
    CHECK(whp_mfc290c_usb_hint_interface(0xffu, 0x01u, 0xffu) ==
          WHP_MFC290C_INTERFACE_VENDOR_CLASS);
    CHECK(whp_mfc290c_usb_hint_interface(0xffu, 0x00u, 0x02u) ==
          WHP_MFC290C_INTERFACE_VENDOR_CLASS);
    /* Potential IF 02 signature only; no physical storage triplet
     * has been reported from the MFC-290C.
     */
    CHECK(whp_mfc290c_usb_hint_interface(0x08u, 0x06u, 0x50u) ==
          WHP_MFC290C_INTERFACE_STORAGE_SCSI_BULK_ONLY);
    CHECK(whp_mfc290c_usb_hint_interface(0x08u, 0x06u, 0x00u) ==
          WHP_MFC290C_INTERFACE_UNCLASSIFIED);
    CHECK(whp_mfc290c_usb_hint_interface(0x08u, 0x05u, 0x50u) ==
          WHP_MFC290C_INTERFACE_UNCLASSIFIED);
    CHECK(whp_mfc290c_usb_hint_interface(0x07u, 0x06u, 0x50u) ==
          WHP_MFC290C_INTERFACE_UNCLASSIFIED);
    CHECK(whp_mfc290c_usb_hint_interface(0x07u, 0x00u, 0xffu) ==
          WHP_MFC290C_INTERFACE_UNCLASSIFIED);
    CHECK(whp_mfc290c_usb_hint_interface(0x08u, 0x01u, 0xffu) ==
          WHP_MFC290C_INTERFACE_UNCLASSIFIED);
    CHECK(whp_mfc290c_usb_hint_interface(0x07u, 0x01u, 0x00u) ==
          WHP_MFC290C_INTERFACE_UNCLASSIFIED);
    CHECK(whp_mfc290c_usb_hint_interface(0x07u, 0x01u, 0x04u) ==
          WHP_MFC290C_INTERFACE_UNCLASSIFIED);

    CHECK(whp_mfc290c_usb_hint_endpoint(0x07u, 0x01u, 0x02u) ==
          WHP_MFC290C_ENDPOINT_PRINTER_BULK_OUT_CANDIDATE);
    CHECK(whp_mfc290c_usb_hint_endpoint(0x07u, 0x81u, 0x02u) ==
          WHP_MFC290C_ENDPOINT_PRINTER_BULK_IN_CANDIDATE);
    CHECK(whp_mfc290c_usb_hint_endpoint(0xffu, 0x01u, 0x02u) ==
          WHP_MFC290C_ENDPOINT_UNCLASSIFIED);
    CHECK(whp_mfc290c_usb_hint_endpoint(0x07u, 0x00u, 0x02u) ==
          WHP_MFC290C_ENDPOINT_UNCLASSIFIED);
    CHECK(whp_mfc290c_usb_hint_endpoint(0x07u, 0x81u, 0x03u) ==
          WHP_MFC290C_ENDPOINT_UNCLASSIFIED);
    CHECK(whp_mfc290c_usb_hint_endpoint(0x07u, 0x71u, 0x02u) ==
          WHP_MFC290C_ENDPOINT_UNCLASSIFIED);

    puts("MFC-290C: offline USB setup/endpoint classifiers passed");
    return 0;
}
