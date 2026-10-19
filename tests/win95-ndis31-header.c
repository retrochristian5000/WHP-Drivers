/* Historical NT 3.51 NDIS miniport type-only check for Windows 95 RTM.
 * This is an object compile probe, NOT a PE .SYS/LE .VXD driver.
 * The DDK include path must be explicitly verified by ./build.
 */
#include <ddk/win95rtm/network/ndis31.h>

#ifndef WHP_WIN95_NDIS31_USE_HISTORICAL_DDK
#error "NDIS 3.1 test requires a verified historical DDK include directory"
#endif
#ifndef NDIS_STATUS_SUCCESS
#error "NDIS 3.1 header does not declare NDIS_STATUS_SUCCESS"
#endif
typedef char whp_w95_ndis31_status_width[(sizeof(NDIS_STATUS) == 4) ? 1 : -1];
typedef char whp_w95_ndis31_handle_width[(sizeof(NDIS_HANDLE) == 4) ? 1 : -1];

unsigned long whp_w95_ndis31_header_probe(void)
{
    return (unsigned long)(NDIS_STATUS_SUCCESS);
}
