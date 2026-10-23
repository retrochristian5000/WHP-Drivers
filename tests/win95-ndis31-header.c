/* Historical NT 3.51 miniport *declaration* probe for Windows 95 RTM.
 * Compile only with an independently verified NT 3.51 DDK include directory.
 * Do not substitute Water's newer NDIS header, Win98/2000, WDM or NDIS 6.
 * This never links a driver, probes PCI, or loads on Windows 95.
 */
#include <ddk/win95rtm/network/ndis31.h>

#ifndef WHP_WIN95_NDIS31_USE_HISTORICAL_DDK
#error "NDIS 3.1 probe requires the independently verified NT 3.51 DDK"
#endif
#ifndef NDIS_STATUS_SUCCESS
#error "Missing historical NDIS_STATUS_SUCCESS"
#endif
#ifndef OID_GEN_SUPPORTED_LIST
#error "Missing NDIS 3.x supported-OID list"
#endif
#ifndef OID_GEN_CURRENT_PACKET_FILTER
#error "Missing NDIS 3.x Ethernet packet-filter OID"
#endif
#ifndef OID_802_3_PERMANENT_ADDRESS
#error "Missing NDIS 3.x Ethernet permanent-address OID"
#endif
#ifndef OID_802_3_CURRENT_ADDRESS
#error "Missing NDIS 3.x Ethernet current-address OID"
#endif

/* Modern-only markers are errors, not permission to use later callbacks. */
#if defined(NDIS_OBJECT_TYPE_MINIPORT_DRIVER_CHARACTERISTICS) || \
    defined(NDIS_MINIPORT_DRIVER_CHARACTERISTICS_REVISION_1)
#error "NDIS 6 miniport registration ABI is not Windows 95 RTM NDIS 3.1"
#endif

typedef char whp_w95_ndis31_status_width[(sizeof(NDIS_STATUS) == 4) ? 1 : -1];
typedef char whp_w95_ndis31_oid_width[(sizeof(NDIS_OID) == 4) ? 1 : -1];
typedef char whp_w95_ndis31_handle_width[(sizeof(NDIS_HANDLE) == 4) ? 1 : -1];
typedef char whp_w95_ndis31_miniport_version_fields[
    (sizeof(((NDIS_MINIPORT_CHARACTERISTICS *)0)->MajorNdisVersion) == 1 &&
     sizeof(((NDIS_MINIPORT_CHARACTERISTICS *)0)->MinorNdisVersion) == 1) ? 1 : -1];
typedef char whp_w95_ndis31_packet_type[(sizeof(NDIS_PACKET) > 0) ? 1 : -1];
/* Verify legacy registration entry points exist; no imports or linkage here. */
typedef char whp_w95_ndis31_wrapper_decl[(sizeof(&NdisMInitializeWrapper) > 0) ? 1 : -1];
typedef char whp_w95_ndis31_register_decl[(sizeof(&NdisMRegisterMiniport) > 0) ? 1 : -1];

NDIS_STATUS whp_w95_ndis31_header_probe(NDIS_HANDLE adapter)
{
    NDIS_MEDIUM medium = NdisMedium802_3;
    NDIS_OID oid = OID_GEN_SUPPORTED_LIST;
    (void)adapter;
    (void)medium;
    (void)oid;
    return NDIS_STATUS_SUCCESS;
}
