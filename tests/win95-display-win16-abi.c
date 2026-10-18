/* Compile-only 16-bit Windows 95 display ABI check.
 * No .DRV exports, DIBENG calls, VDD service calls, or GPU register access.
 */
#include <whp/win95rtm/display/win16_abi.h>
#ifdef WHP_WIN95_DISPLAY_USE_HISTORICAL_DDK
#include <whp/win95rtm/display/ddk_minidrv.h>
#endif

unsigned short whp_win95_display_header_probe(void)
{
    return (unsigned short)(sizeof(WORD) + sizeof(DWORD) + sizeof(LPSTR));
}
