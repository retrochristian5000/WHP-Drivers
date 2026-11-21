#include "probe_scope.h"

/* Evidence source is not a driver claim. DOS PCI BIOS enumerates USB
 * host controllers, not USB peripherals. IOKit reports registry hints,
 * not authenticated raw device/interface descriptors.
 */
const char *whp_probe_scope_line(enum whp_probe_scope scope)
{
    switch (scope) {
    case WHP_PROBE_SCOPE_DOS_BIOS_PCI:
        return "WHP probe platform=dos evidence=BIOS-PCI usbDevices=not-enumerated transfers=no";
    case WHP_PROBE_SCOPE_MACOS_USB_REGISTRY:
        return "WHP probe platform=macos evidence=IORegistry-CUPS usbDevices=registry-only transfers=no";
    case WHP_PROBE_SCOPE_MACOS_ARCHITECTURE:
        return "WHP probe platform=macos evidence=process-architecture usbDevices=not-queried transfers=no";
    case WHP_PROBE_SCOPE_MACOS_SYSTEM:
        return "WHP probe platform=macos evidence=sysctl-IORegistry usbDevices=not-queried transfers=no";
    default:
        return 0;
    }
}
