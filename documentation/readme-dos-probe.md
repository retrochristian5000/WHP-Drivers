# WHP-Drivers — WHPPROBE DOS BIOS, PCI and memory diagnostics

This document preserves detailed research moved from the root README. Commands and relative source paths in code blocks are relative to the **repository root**. See [the project overview](../README.md) for the shortest supported entry points.

### DOS system probe: PCI/PCIe and legacy BIOS resources (2026-11-18)

**Tool name:** `WHPPROBE.EXE` (8.3-compatible). The program remains a
single, read-only DOS MZ executable with separate PCI and BIOS legacy
probe paths. The original `WHPPCI.EXE` is no longer generated,
but existing saved `dos-pci` profiles and `./build dos-pci-build`
are accepted as compatibility aliases. The canonical new
profile/command is `dos-probe` / `./build dos-probe-build`.
The generated artifacts retain the existing
`.build/dos-pci/` directory, avoiding a duplicate build tree.
Previously built `WHPPCI.EXE` files in that folder are historical
artifacts, not aliases for the current executable; the build does not
silently delete them.

#### Shared probe core — DOS stays DOS-bound

The DOS probe and macOS observer share **only**
`tools/probe/probe_scope.{h,c}`, a C89-compatible reporting
function describing the evidence source and its limitations.
A DOS report now begins with a common `WHP probe platform=dos`
scope line, explicitly stating that USB **peripherals** were not
enumerated. This changes the report *preamble*, not its original
BIOS/PCI observations, menu options or data collection.

All 16-bit OpenWatcom, BIOS interrupt, BDA, PCI BIOS and DOS
console logic remains in `tools/dos/`. The main DOS build
links the shared pure-C module as a 16-bit DOS object and still
produces `WHPPROBE.EXE`; it does not link IOKit or CUPS.
On macOS, the same source compiles into its native
`mfc290c-probe` process, without any DOS headers or runtime.

The portable `./build test-probe-scope` regression checks all
three scope reports (DOS, macOS USB, macOS architecture-only) and
rejects unknown scope values. It runs from both existing DOS-menu
and Brother MFC-290C host regression paths. Passing host tests
cannot substitute for an actual DOS MZ link or DOS execution;
the separate DOS CI workflow verifies linking and packaging.

#### DOS APIs and native macOS replacements

The macOS system companion is run through
`./build.sh probe-system-macos`. It reports Darwin's kernel
release using `uname()`, installed physical RAM using
`sysctlbyname("hw.memsize")`, and published `IOPCIDevice`
and `IOSerialBSDClient` service counts using IOKit.
These are **native observations**, not substitute values
for DOS INT 12h's conventional-memory boundary, the
INT 11h BIOS equipment word, or the INT 1Ah PCI BIOS
configuration-space reads. A zero IOKit service count
does not establish that no corresponding hardware exists.

The DOS tool continues using BIOS, the BDA, the DOS
video/menu APIs and OpenWatcom 16-bit addressing. Neither
macOS tool has a DOS BIOS dependency. The generic Mac
tool reports no USB peripheral enumeration; the Brother
tool still handles its own read-only USB-device registry
inspection. The two executables are separate.

See the [per-API compatibility map](probe-api-portability.md)
for supported replacements, non-equivalences, safety limits
and validation gates.

#### GitHub Actions DOS source build and archives

The separate [DOS source-build workflow](../.github/workflows/dos-probe-build.yml)
checks out the pinned OpenWatcom gitlink, bootstraps its 16-bit compiler,
runs the host-only DOS regressions, builds `WHPPROBE.EXE` using the existing
`dos-probe-build` target, and verifies its MZ structure. It publishes
`WHPPROBE-<commit12>.tgz` and `WHPPROBE-<commit12>.zip` in one short-lived
GitHub Actions artifact. Both formats include the identical 16-bit
`WHPPROBE.EXE`, DOS-compatible `README.TXT`, and `BUILD.TXT` with the
exact repository and OpenWatcom commit IDs and executable SHA-256.
`SHA256SUMS` accompanies the two archives outside them. The `.tgz`
suffix is the conventional shorter form of `.tar.gz`.

The built-in packager uses Python standard-library TAR/GZIP/ZIP writers;
it refuses invalid MZ binaries and symlinked executable inputs and uses
stable archive metadata. Run its synthetic regression without any compiler:
`./build test-dos-packages`. Packaging tests do **not** prove successful
OpenWatcom linking or DOS execution. CI never installs or runs the DOS
executable on the Ubuntu host; real DOS and physical hardware remain untested.

#### COM1-COM4 serial-port evidence

The default batch report (`WHPPROBE /R`) and legacy BIOS menu list all
four standard COM address slots from the BIOS Data Area (0040:0000-0006).
The INT 11h equipment serial count and nonzero BDA addresses are separate
firmware observations. The probe flags disagreements and duplicate address
assignments without claiming that a working UART is present.

To explicitly request BIOS serial status, use `WHPPROBE /S` or press
**S** in menu section 2. For plausible, unique BDA bases, this uses
INT 14h AH=03h to read the BIOS line and modem status; it never initializes
the UART, sends data, receives data or writes directly to serial I/O ports.
BIOS status reads may acknowledge latched UART bits, so avoid this
optional operation during active serial communications. A response does
not prove functional hardware or an attached serial device.

The standard PCI report additionally flags PCI class 07:00 serial and
07:02 multiport controller candidates. PCI functions are not silently
assigned COM numbers. USB serial adapters, ISA PnP, PCI driver mapping
and live interrupt/baud/loopback checks are not yet supported; an empty
BDA entry does not prove the corresponding hardware is absent.

#### ISA and legacy PC-compatible probing

The first ISA-related stage uses two independent, read-only mechanisms:

- **BIOS `INT 11h`** equipment word: BIOS-reported serial/parallel
  counts, floppy count, mathematics coprocessor flag, game-port bit,
  and initial video-mode code. Values are historical flags, may be
  absent or stale on newer systems, and are not a definitive hardware
  inventory.
- **BIOS Data Area `0040:0000`**: four 16-bit COM base-address
  fields at offsets `00h`–`06h`; three LPT base-address fields
  at `08h`–`0Ch`. Zero is reported as no firmware address,
  not as proof of no serial/parallel hardware. The executable reads
  those words using real-mode Watcom `MK_FP`; it never accesses the
  listed I/O ports.
- **PCI bridge class `06:01`**: displays ISA-compatible or LPC
  bridge *candidates* when visible via PCI BIOS, without assuming
  that a physical ISA slot exists or that the bridge routes a
  particular legacy address. This is distinct from a type-1 PCI
  bridge (class `06:04`).

The BIOS legacy section runs **before** the optional PCI BIOS preflight,
so an absent `INT 1Ah` PCI BIOS does not erase those observations.
The existing nonzero exit code **2** still reports PCI BIOS
unavailability, meaning the combined scan was **incomplete**, rather
than indicating absence of every bus. Existing PCIe, USB, storage,
Radeon/Realtek and bridge decoders remain read-only.

**Not yet implemented:** ISA Plug and Play BIOS node enumeration,
CSN/resource isolation, direct 8237 DMA/8259 PIC/8042 probes, EISA
slot configuration, PCMCIA, ACPI tables, SMBus transactions and
verified physical slot/card identification. Never try ISA cards
by writing to unassigned ports or by assuming fixed IRQ/DMA values:
these actions can collide with integrated southbridge functions.
Real-mode Windows 95 DOS and pure MS-DOS may expose different firmware
services; neither a BIOS resource flag nor a PCI function guarantees
a working Windows driver.

Use host-only synthetic tests and the shared profile:

```sh
./build test-dos-legacy-bios
WHP_DRIVER_PROFILE=dos-probe ./build.sh test-fast
./build.sh test-menuconfig
```

Evidence: BIOS equipment list and BDA mapping are documented in
[Ralf Brown's Interrupt List](https://www.delorie.com/djgpp/doc/rbinter/it/26/2.html)
and its [BIOS data segment layout](https://dose.0wnz.at/ng/rbrown/ng19b2.php);
Watcom's [`MK_FP` interface](https://openwatcom.github.io/open-watcom-v2-wikidocs/cpguide.html)
is confined to the low-memory BIOS Data Area in the DOS-specific
entrypoint. Pure decoding in `tools/dos/legacy_bios.[ch]` is
host-testable without Watcom or a physical ISA device.

A **separate 16-bit real-mode MS-DOS target** now provides a bridge
between our portable PCI decoders and a machine's firmware. It is
`tools/dos/whpprobe.c`, linked as `WHPPROBE.EXE` using the explicitly
selected, pinned OpenWatcom 16-bit DOS compiler and DOS runtime.
It is **not** a TSR, DOS device driver, PCI.VXD replacement, NIC
miniport or chipset initialization code.
The DOS linker reserves an explicit 8 KiB conventional-memory stack
for the C runtime and BIOS calls; that footprint is a deliberate cost.

#### DOS kernel version and API boundary (6.22 / 7.00 / 7.10)

`WHPPROBE.EXE` now prints both the version reported by
`INT 21h, AH=30h` (subject to `SETVER`) and, where the call is
credible, the kernel version returned by `INT 21h, AX=3306h`.
The small pure-C classifier lives in `tools/dos/dos_version.[ch]`,
with hardware-free tests in `tests/dos-version.c`. An unavailable or
suspicious true-version result is marked **unverified**; the reported
number is not promoted to an API capability claim.

Keep three historical boundaries distinct:

- **MS-DOS 6.22**: FAT12/FAT16 and conventional 8.3 filename APIs;
  no native FAT32 or Win9x `71xxh` long-filename services.
- **MS-DOS 7.00**: Windows 95 RTM / SP1 real-mode kernel; do not
  assume the later FAT32 functions of Windows 95 OSR2.
- **MS-DOS 7.10**: Windows 95 OSR2 / Windows 98 real-mode kernel,
  with FAT32 extensions; `INT 21h, AX=73xxh` support must still
  be checked per service. The Windows DOS prompt and true MS-DOS
  mode are different environments. `71xxh` long-filename calls
  can be supplied by the running Windows IFSMgr and must be
  feature-probed (unsupported `AX=7100h`), not inferred from 7.10.

These are version profiles, **not proof of Microsoft OEM identity**.
DOS 8.00 / Windows Me and alternative DOS kernels remain separate.
No new FAT32, long-filename, disk-write or device-init calls were
added to the PCI utility; it still uses read-only `INT 1Ah` PCI
BIOS services regardless of DOS version. This is diagnostic
instrumentation, not a claim that PCIe, HDA or the RTL8111E work.

```sh
./build test-dos-version                  # host-only; no DOS interrupt
WHP_DRIVER_PROFILE=dos-probe ./build.sh test-fast
```

#### DOS PCIe memory preflight (read-only)

`WHPPROBE.EXE` can discover PCIe endpoints via firmware's **standard**
PCI configuration space but **does not provide a protected-mode driver
or access to their memory-mapped registers**. The new
`drivers/bus/pci95/pci_dos_memory.[ch]` logic inspects BAR base
addresses and type-1 bridge memory windows already returned by the
PCI BIOS. It never probes BAR sizes by writing all-ones values,
reconfigures bridges, enables bus mastering, allocates DMA memory,
maps physical addresses, or reads/writes PCIe MMIO.

The DOS executable also calls **BIOS INT 12h** to report the installed
conventional-memory upper boundary. This is *not* a measurement of
free memory, UMB availability, XMS, EMS, or the physical e820 map.
Warnings distinguish **where a BAR begins** from whether an entire
BAR aperture actually overlaps RAM: the size is unknown without
additional trusted resource information. The bridge audit uses actual
base/limit registers and PCI Command.Memory Space Enable to distinguish
declared windows from active decoding. A potential collision is a
diagnostic clue, not proof of corruption.

- I/O BARs within and beyond the x86 16-bit I/O-port addressing range
  are distinguished; no port I/O is performed.
- 32-bit and 64-bit MMIO BARs retain full base addresses.
- Bases below the BIOS conventional-memory bound receive a warning.
  Bases in the remaining low 1 MiB are flagged as legacy-address
  candidates; neither implies the firmware necessarily maps RAM there.
- Above-1-MiB MMIO requires a deliberate physical mapping mechanism,
  not a cast to a real-mode far pointer. Addresses above 4 GiB require
  additional address-width and mapping support.
- Bridge memory windows are checked for low-memory ranges, address
  ranges crossing 4 GiB, and inactive memory decoding. The decoder
  does not claim that a reported bridge window's range is currently
  accessible from DOS.

The existing `WHPPROBE.EXE` MZ image reserves an 8-KiB stack.
Ralf Brown's Interrupt List notes that a PCI BIOS `INT 1Ah, AX=B109h`
read can consume up to **1,024 bytes** of stack; the real-mode runtime,
compiler-generated frames and any resident software must still be
validated in an actual DOS guest. This work neither shrinks the stack
nor adds a memory allocator or a speculative VCPI/DPMI mode switch.

Use the light host-only checks:

```sh
./build test-pci-dos-memory
WHP_DRIVER_PROFILE=dos-probe ./build.sh test-fast
```

**Boundary:** PCI BIOS `B109h` reads only standard configuration
offsets `00h`–`FFh` in this tool, not PCIe extended configuration.
The BIOS might omit legacy PCI BIOS support even when PCIe hardware is
present. MS-DOS 6.22 and DOS 7.10 use the same *BIOS diagnostic*
interface here; their different filesystem APIs are independent.
Driver-level PCIe support still needs a verified mapping model,
resource ownership, IRQ handling and safe DMA bounce/translation
policies before any live hardware writes.

Sources:
- Ralf Brown, PCI BIOS read-configuration-word contract and stack note:
  https://fd.lod.bz/rbil/interrup/bios_expansion/1ab109.html
- PCI bridge memory register layout:
  https://sources.debian.org/src/seabios/0.5.1-3/src/pci_regs.h
- Windows PCIe extended configuration-space distinction:
  https://learn.microsoft.com/en-us/windows-hardware/drivers/pci/accessing-pci-device-configuration-space

The utility checks the historical PCI BIOS installation function
`INT 1Ah, AX=B101h` and uses the **read-only** configuration-word
function `AX=B109h` to enumerate functions up through the BIOS-reported
last PCI bus. It reconstructs standard PCI configuration snapshots in
little-endian order and reuses the existing type-1 bridge and RTL8111E
identity parsers. It prints host bridge identities, PCI-to-PCI bridge
primary/secondary/subordinate bus numbers and declared resource
windows, and `10EC:8168` Ethernet-family endpoint evidence. PCIe
standard capability offsets are checked when the firmware supports
the full standard 256-byte configuration range. No direct config I/O
ports (CF8/CFC), bus-number or BAR writes, DMA, MMIO, interrupt
initialization or device installation are performed. The 16-bit
installation preflight sees only the lower half of the 32-bit PCI BIOS
signature, along with firmware status and version: it does not
authenticate firmware. The full PCIe **extended** 4096-byte
configuration space is not examined.

**Why DOS first?** Testing from Windows 95's **Restart in MS-DOS Mode**
can show whether the firmware exposes the NIC and its upstream bridges
independently of Windows 95 protected-mode PCI.VXD and NDIS. A DOS
prompt *inside* a running Windows 95 desktop is not equivalent.
A DOSBox guest examines the emulator's virtual firmware and PCI bus,
not the physical Pavilion.
Windows 95 can also run some real-mode NDIS 2 network adapter drivers
through `NDIS2SUP.VXD`, but that compatibility path has its own
conventional-memory, mode-transition and configuration costs. It is
**not** equivalent to an NDIS 3.1 RTL8111E miniport and does not make
an unenumerated PCIe controller automatically usable.
 A PCI BIOS failure does not prove there
is no PCIe hardware: a firmware implementation may omit this legacy
interface. Successful DOS visibility also does not establish that
Windows 95 RTM enumerates the device or has working IRQ/resource routing.

To test the host-side decoder and format regressions **without**
OpenWatcom or LLVM:

```sh
WHP_DRIVER_PROFILE=dos-probe ./build.sh test-fast
./build test-pci95-bios-read
./build test-dos-mz
```

To build the **DOS MZ executable** after preparing the pinned
OpenWatcom source and compiler (see the existing independent
`bootstrap-watcom` section):

```sh
./build.sh bootstrap-watcom            # explicit, potentially costly
WHP_DRIVER_PROFILE=dos-probe ./build.sh plan
WHP_DRIVER_PROFILE=dos-probe ./build.sh incremental
# generated: .build/dos-pci/WHPPROBE.EXE
./build.sh clean-dos-probe              # cleans only DOS output artifacts
```

#### Read-only memory-resource survey

Choose **DOS version and memory** from the WHPPROBE menu, then press
**M** to run the on-demand memory resource survey. It compares
firmware-reported memory boundaries with PCI BAR start addresses, without
scanning the bus simply because the DOS-version page was opened.
The report gathers INT 12h, BDA 0040:0013h, the tentative EBDA segment
at 0040:000Eh (which may also be LPT4), and INT 15h AH=88h as a
**legacy, incomplete** extended-memory estimate. A missing value is
reported as unknown; inconsistent conventional/EBDA boundaries are
flagged rather than silently merged.

```text
WHPPROBE.EXE /M > MEMORY.TXT
```

The memory scan uses the preexisting read-only PCI BIOS transport and
BAR decoder. Only **enabled PCI MMIO BAR base addresses** are checked
against the credible conventional limit; raw BAR size and the actual
memory allocation are unknown. A suspicious base inside conventional
RAM is a **potential conflict**, not proof of a faulty driver. EBDA,
low-memory gaps, the VGA aperture and ROM/UMB areas are distinguished
without automatically treating intentional hardware mappings as errors.

This does **not** sweep or modify RAM, test for failing DIMMs, locate
all DOS allocations, inspect EMS/XMS or drivers' DMA buffers, or obtain
the modern BIOS E820 physical-memory map. Windows 95 driver ownership
and physical memory stability remain unverified. The /R and /A reports
are unchanged; the memory survey is an additional opt-in report.

#### Interactive DOS diagnostics menu

`WHPPROBE.EXE` now opens an MSD-style **read-only** text menu when standard
input and output are console devices. Navigation: Up/Down or numbers 1-9
select DOS/memory, legacy BIOS, PCI bridges, display, network, HDA,
storage, USB, or all PCI functions. Enter opens a category; Esc or Q
returns to the menu. PCI devices are shown individually: R displays
the function's standard PCI configuration registers, while Enter/N
advances to the next device. The menu does not require ANSI.SYS.

Redirected input or output automatically retains the old streaming
report mode, so existing `WHPPROBE.EXE > PCI.TXT` scripts still work.
Force a conventional report with `/R` or all PCI registers with `/A`:

```text
WHPPROBE.EXE
WHPPROBE.EXE /R > PCI.TXT
WHPPROBE.EXE /A > ALLPCI.TXT
WHPPROBE.EXE /?
```

Interactive PCI detail pages read full 256-byte PCI BIOS configuration
snapshots **only when R is selected** and fall back to the saved
64-byte header on a failed read. No BAR sizing writes, MMIO access,
driver installation, or hardware initialization is performed.
Host-side synthetic menu tests do not substitute for an OpenWatcom
16-bit build and emulator or hardware execution.
With a DOS-compatible, BIOS-based Pavilion boot environment,
**copy** `WHPPROBE.EXE` to removable media, then run from genuine DOS
or Windows 95 restarted into MS-DOS Mode:

```text
WHPPROBE.EXE > PCI.TXT
```

Examine each type-1 bridge's observed primary/secondary/subordinate
bus range and compare it with the bus where the Realtek endpoint is
actually enumerated. Do not assume bus 3 (or any other fixed bus
number) from a past observation. Compare the read-only snapshot with
historical LISTPCI and Windows 95 Device Manager evidence.
`TxConfig` offset `0x40` belongs to the Realtek NIC's **device
register map**, *not* to PCI configuration offset `0x40` and is never
read by this program. A `10EC:8168` identity or PCI revision byte
therefore does not by itself establish an RTL8111E silicon revision.

The read transport (`pci_bios_read.[ch]`) accepts a mockable word
reader. Its synthetic host regressions cover success, missing
devices, unsupported sizes, little-endian decoding, partial failures
and output preservation; the independent MZ checker rejects
structurally invalid DOS executables. An actual DOS target build
requires pinned native OpenWatcom binaries **and** DOS runtime
libraries; a source checkout does not suffice. No successful
Pavilion DOS run or authentic Win95 PCI bridge driver is claimed.

The default ABI/RTL/ALL workflows retain their previous software
targets; the DOS executable build is **opt-in** so an ordinary `all`
build does not silently incur the OpenWatcom DOS toolchain cost.

Historical contracts:
- PCI BIOS v2.x INT 1Ah: https://fd.lod.bz/rbil/interrup/bios_expansion/1ab101.html
- Read Configuration Word B109h: https://www.delorie.com/djgpp/doc/rbinter/id/86/23.html
- OpenWatcom 16-bit DOS tools: https://open-watcom.github.io/open-watcom-v2-wikidocs/cguide.html
- Windows 95 Restart in MS-DOS Mode: https://www.betaarchive.com/wiki/index.php/Microsoft_KB_Archive/138996
- NDIS 2 real-mode compatibility via NDIS2SUP.VXD: https://techshelps.github.io/MSDN/DNWIN95/HTML/S637B.HTM
