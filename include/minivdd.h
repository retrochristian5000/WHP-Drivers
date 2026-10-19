/* Project-owned MINIVDD.H for WHP Windows 95 display compile probes.
 *
 * This is a limited, cross-version-referenced C compatibility header,
 * NOT Microsoft's original Win95 RTM MINIVDD.H or MINIVDD.INC.
 * No callable VDD entry points, dispatch structures, or driver binaries
 * are supplied. A verified historical DDK include path takes priority.
 */
#ifndef WHP_WIN95_MINIVDD_COMPAT_H
#define WHP_WIN95_MINIVDD_COMPAT_H

#include <ddk/win95rtm/display/minivdd.h>

#define MINIVDD_SVC_BASE_OFFSET           WHP_W95_VDD_API_BASE
#define VDD_DRIVER_REGISTER               WHP_W95_VDD_DRIVER_REGISTER
#define VDD_DRIVER_UNREGISTER             WHP_W95_VDD_DRIVER_UNREGISTER
#define VDD_SAVE_DRIVER_STATE             WHP_W95_VDD_SAVE_DRIVER_STATE
#define VDD_REGISTER_DISPLAY_DRIVER_INFO  WHP_W95_VDD_REGISTER_DRIVER_INFO
#define VDD_REGISTER_SSB_FLAGS            WHP_W95_VDD_REGISTER_SSB_FLAGS
#define VDD_GET_DISPLAY_CONFIG            WHP_W95_VDD_GET_DISPLAY_CONFIG
#define VDD_PRE_MODE_CHANGE               WHP_W95_VDD_PRE_MODE_CHANGE
#define VDD_POST_MODE_CHANGE              WHP_W95_VDD_POST_MODE_CHANGE
#define VDD_SET_USER_FLAGS                WHP_W95_VDD_SET_USER_FLAGS
#define VDD_SET_BUSY_FLAG_ADDR            WHP_W95_VDD_SET_BUSY_FLAG_ADDR

#define REGISTER_DISPLAY_DRIVER           WHP_W95_MINI_REGISTER_DISPLAY_DRIVER
#define PRE_HIRES_TO_VGA                  WHP_W95_MINI_PRE_HIRES_TO_VGA
#define PRE_VGA_TO_HIRES                  WHP_W95_MINI_PRE_VGA_TO_HIRES
#define ENABLE_TRAPS                      WHP_W95_MINI_ENABLE_TRAPS
#define DISABLE_TRAPS                     WHP_W95_MINI_DISABLE_TRAPS
#define GET_CHIP_ID                       WHP_W95_MINI_GET_CHIP_ID

#define RETURNED_DATA_IS_STALE            WHP_W95_DISPLAYINFO_STALE
#define MINIVDD_FAILED_TO_LOAD            WHP_W95_DISPLAYINFO_MINIVDD_LOAD_FAILED
#define MINIVDD_CHIP_ID_DIDNT_MATCH       WHP_W95_DISPLAYINFO_CHIP_MISMATCH
#define MONITOR_INFO_NOT_VALID            WHP_W95_DISPLAYINFO_MONITOR_INVALID

/* Historical name exposed only by the compatibility shim; the WHP header
 * keeps its name separate to avoid silently mixing the two interfaces.
 */
typedef WHP_W95_DISPLAYINFO DISPLAYINFO;

#endif /* WHP_WIN95_MINIVDD_COMPAT_H */
