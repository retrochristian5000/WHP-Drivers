/* No DOS interrupts, IOKit, CUPS or USB hardware. C89-compatible test. */
#include "probe_scope.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "probe scope line %d: %s\n", __LINE__, #expr); \
        return 1; \
    } \
} while (0)

int main(void)
{
    const char *dos = whp_probe_scope_line(WHP_PROBE_SCOPE_DOS_BIOS_PCI);
    const char *mac = whp_probe_scope_line(WHP_PROBE_SCOPE_MACOS_USB_REGISTRY);
    const char *arch = whp_probe_scope_line(WHP_PROBE_SCOPE_MACOS_ARCHITECTURE);

    CHECK(dos != NULL && mac != NULL && arch != NULL);
    CHECK(strcmp(dos, "WHP probe platform=dos evidence=BIOS-PCI "
                      "usbDevices=not-enumerated transfers=no") == 0);
    CHECK(strcmp(mac, "WHP probe platform=macos evidence=IORegistry-CUPS "
                      "usbDevices=registry-only transfers=no") == 0);
    CHECK(strcmp(arch, "WHP probe platform=macos evidence=process-architecture "
                       "usbDevices=not-queried transfers=no") == 0);
    CHECK(whp_probe_scope_line((enum whp_probe_scope)0) == NULL);
    CHECK(whp_probe_scope_line((enum whp_probe_scope)4) == NULL);
    CHECK(whp_probe_scope_line((enum whp_probe_scope)-1) == NULL);

    puts("WHP probe: common source-scope tests passed");
    return 0;
}
