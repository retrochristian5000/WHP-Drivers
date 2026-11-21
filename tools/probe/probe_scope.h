/* Cross-platform WHP probe source reporting. No operating-system calls. */
#ifndef WHP_PROBE_SCOPE_H
#define WHP_PROBE_SCOPE_H

enum whp_probe_scope {
    WHP_PROBE_SCOPE_DOS_BIOS_PCI = 1,
    WHP_PROBE_SCOPE_MACOS_USB_REGISTRY,
    WHP_PROBE_SCOPE_MACOS_ARCHITECTURE
};

/* Newline-free source label; returns NULL for invalid input. */
const char *whp_probe_scope_line(enum whp_probe_scope scope);

#endif
