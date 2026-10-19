/* Compile-only 16-bit Windows 95 display ABI check.
 * No .DRV exports, DIBENG calls, VDD service calls, or GPU register access.
 */
#include <ddk/win95rtm/win16_abi.h>
#include <minivdd.h>
/* Win16 sees the same driver/VDD constants without importing NT headers. */
typedef char whp_w95_win16_vdd_register[(VDD_DRIVER_REGISTER == 0x80) ? 1 : -1];
typedef char whp_w95_win16_displayinfo[(sizeof(DISPLAYINFO) == 50) ? 1 : -1];
#ifdef WHP_WIN95_DISPLAY_USE_HISTORICAL_DDK
#include <ddk/win95rtm/display/ddk_minidrv.h>
#endif

unsigned short whp_win95_display_header_probe(void)
{
    return (unsigned short)(sizeof(WORD) + sizeof(DWORD) + sizeof(LPSTR));
}
