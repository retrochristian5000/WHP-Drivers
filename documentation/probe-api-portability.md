# WHP probe: DOS API migration to native macOS

**Implementation status (2026-11-21):** Shared portable source
reporting, a 16-bit DOS BIOS/PCI probe, a native macOS
Brother IOKit/CUPS observer, and a new native macOS
system observer. These are **not** a DOS API emulator or
interchangeable binaries.

## Exact API and semantic map

| DOS call or construct in WHPPROBE | macOS route | Verified implementation boundary |
| --- | --- | --- |
| \`INT 21h AH=30h\`, \`AX=3306h\` (DOS version) | \`uname()\`, Darwin kernel-release report | **Implemented** in \`whp-system-probe\`; this is *not* a DOS or marketing macOS version |
| \`INT 12h\`, BDA \`0040:0013h\` (conventional-memory boundary) | \`sysctlbyname("hw.memsize")\` | **Implemented**, reports physical installed RAM in bytes—not DOS conventional memory, free memory, UMB, EMS or XMS |
| \`INT 15h AH=88h\`, EBDA BDA pointer | macOS VM/sysctl information where appropriate | **Not equivalent**; do not simulate 16-bit segmented memory or the EBDA |
| \`INT 1Ah AX=B101h/B109h\` (PCI BIOS presence/config word) | IOKit \`IOPCIDevice\` inventory | **Implemented only as a bounded read-only registry-service count**. No config-register, MMIO, BAR or IRQ reads |
| \`INT 11h\` plus BDA COM/LPT words | IOKit \`IOSerialBSDClient\` matching | **Implemented only as a bounded read-only serial-service count**; not COM1–COM4 address discovery and not legacy LPT BIOS flags |
| \`INT 14h AH=03h\` (serial status) | IOKit serial discovery; \`termios\` for separately authorized active port operations | Active status query **not implemented**; no device opened or data consumed |
| \`INT 10h AH=0Fh/06h/02h\` (video/menu) | Terminal output, capabilities and \`termios\` | **Not migrated**; do not call video BIOS from macOS |
| DOS \`getch()\` / \`<conio.h>\` | \`termios\` plus \`read()\` in a future Mac UI | **Not migrated**; preserve DOS extended-key behavior in DOS only |
| \`MK_FP()\`, \`__far\`, segment \`0040h\` | No safe equivalent in macOS user space | **DOS-only**, never map arbitrary physical addresses |
| \`stdio\`, \`isatty(fileno(...))\` | Portable C/POSIX calls | Can share presentation semantics, but not DOS hardware reads or terminal mode assumptions |

## Repository ownership

The shared \`tools/probe/probe_scope.{h,c}\` uses
C89-compatible code without OS APIs and reports the
scope of each backend. \`tools/dos/whpprobe.c\` retains
all BIOS calls and OpenWatcom-specific code and links
only into the real-mode \`WHPPROBE.EXE\` image. The
native \`tools/macos/whp_system_probe.c\` uses
\`uname\`, read-only \`hw.memsize\`, and bounded IOKit
matching/iteration. \`tools/macos/mfc290c_probe.c\`
continues to enumerate the Brother USB device through
IOKit and CUPS independently.

\`\`\`sh
./build test-probe-scope
./build build-brother-mfc290c-macos
./build.sh probe-system-macos
./build.sh probe-brother-mfc290c-macos
\`\`\`

The macOS build produces two distinct Mach-O executables,
both verifying the selected architecture slices. The DOS
target remains an OpenWatcom 16-bit MZ executable.
No USB transfers, serial I/O, PCI config reads, driver
binding or hardware state changes are introduced.

**Validation boundary:** The shared source-label tests
are host-only. CI on macOS checks the system observer's
reported scope, kernel/memory keys and IOKit count keys;
a zero registry count is allowed. Only the separate DOS
CI build can certify DOS MZ linking, and even that does
not prove real-DOS execution. Neither CI runner has the
physical MFC-290C attached.

## Reference API contracts

- Apple: [Determining system capabilities, \`hw.memsize\`](https://developer.apple.com/documentation/kernel/1387446-sysctlbyname/determining_system_capabilities)
- Apple: [IOPCIDevice service type](https://developer.apple.com/documentation/kernel/iopcidevice)
- Apple: [Serial-port IOKit discovery](https://developer.apple.com/documentation/iokit/communicating_with_a_modem_on_a_serial_port)
- Apple: [IOKit device matching](https://developer.apple.com/library/archive/documentation/DeviceDrivers/Conceptual/IOKitFundamentals/Matching/Matching.html)
- macOS: [sysctl(3) reference](https://man.freebsd.org/cgi/man.cgi?query=sysctl&sektion=3&manpath=macOS+26.7)
