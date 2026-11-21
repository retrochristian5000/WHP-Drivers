# WHP-Drivers — AMD HDA, Windows sound and display, Radeon

This document preserves detailed research moved from the root README. Commands and relative source paths in code blocks are relative to the **repository root**. See [the project overview](../README.md) for the shortest supported entry points.

## HD Audio / AMD FCH Azalia research target (2026-11-18)

The HP Pavilion p6-2310 (H3Y76AA) product specification advertises
**High Definition Audio with up to 5.1 channels** and the **AMD A55
Fusion Controller Hub (FCH)**. The AMD FCH Azalia PCI controller
`1022:780D` is a **hardware-family candidate**, not yet a confirmed
Pavilion-specific PCI device. Additional HDA-class devices (for
example, display/HDMI audio) must remain separate PCI functions.
The **onboard HDA codec identity and widget topology are unknown**.
Do not infer an ALC662/ALC8xx/IDT/Conexant codec from an FCH PCI ID.
In particular, third-party listings naming Realtek **ALC655/656**
conflict with HDA assumptions: ALC655 is an **AC'97** codec.
Never interchange AC'97 registers, Intel HDA controller registers,
or HDA codec verbs without a verified hardware witness.

`drivers/audio/hda/hda_probe.[ch]` is a real **source-level research
module**, but not a usable sound driver. Its first layer decodes
externally captured 64-byte PCI type-0 configuration snapshots and
accepts only standard HDA class/subclass/programming interface
`04:03:00`. It retains vendor/device/revision/subsystem IDs,
PCI command/status, IRQ line/pin and BAR0/BAR1 evidence. BAR decoding
distinguishes unassigned, I/O, invalid, 32-bit MMIO and 64-bit MMIO
resources; decoded addresses are **not valid mappings**. The class
match cannot prove sound output or codec identity.

The second layer decodes **separately acquired 96-byte HDA MMIO
snapshots**, with GCAP, HDA spec version, GCTL, STATESTS, stream
counts, serial data outputs, 64-bit DMA addressing and reported CORB/
RIRB ring-size capability bits. This snapshot is **not** PCI
configuration space, and must be provided by a separately validated
OS/firmware mapping. These read-only values are not evidence of
initialized DMA engines, connected audio pins, valid amplifier
routing, or functioning interrupt delivery. The probe does not
reconfigure GCTL, reset HDA, touch wake/status W1C bits, submit
codec verbs, initialize CORB/RIRB, turn on bus mastering, install
drivers, or play sound.

Run the cheapest regression route first (host C compiler; no LLVM,
OpenWatcom, DOS emulator or actual sound hardware):

```sh
./build test-hda-probe
WHP_DRIVER_PROFILE=hda ./build.sh test-fast
WHP_DRIVER_PROFILE=hda ./build.sh plan
```

To also verify the portable source compiles into an **i386 Windows
COFF object** using the already built, pinned WHP LLVM:

```sh
WHP_DRIVER_PROFILE=hda ./build.sh incremental
```

### Pinned Water audio/HDA header integration (2026-11-18)

The existing `third_party/water` submodule is the authoritative
source for Windows 95 RTM **compile-time** driver reference headers.
The gitlink is advanced to include:
- `include/ddk/hda/hda_regs.h`: OS-neutral Intel HDA PCI class
  tuple, GCAP/GCTL/STATESTS, CORB/RIRB offsets and bit masks
- `include/ddk/win95rtm/audio/hda_regs.h`: 37 legacy aliases for the
  shared register values; **not** evidence that Windows 95 shipped HDA
- `include/ddk/win95rtm/win16_abi.h`: neutral
  `WHP_WIN95_RTM_VERSION` for audio and display, preserving
  `WHP_WIN95_RTM_DISPLAY_VERSION` as an alias
- The unchanged `audio/ddk_audio.h` Win16 boundary and
  `vmm.h` i386 DDB reference; neither is a complete historical DDK

This matters because Water models Windows releases separately:
XP RTM lacked bundled UAA HDA; XP SP1/SP2 could gain the separately
installed KB888111 update; XP SP3 includes UAA HDA bus support, while
Windows 95 RTM remains without a native HDA driver. Neither PCI
detection nor a UAA bus driver proves that the appropriate codec
driver has bound and audio output is working. The historical matrix
is in Water's `documentation/windows-xp-hda-uaa.md`. The existing
HDA research must not silently become an XP, Vista or Win95 driver.

The HDA *software* decoder remains standalone, so cheap unit tests
are unaffected by a missing Water checkout. The targeted opt-in
contract uses `tests/hda-water-header.c` and requires the pinned Water
checkout, never arbitrary host headers or a moving `master` fallback:

```sh
git submodule update --init third_party/water
./build test-hda-water          # native C cross-header check, no LLVM
WHP_DRIVER_PROFILE=hda ./build.sh test-fast   # standalone HDA data tests
WHP_DRIVER_PROFILE=hda ./build.sh incremental # pinned LLVM + Water checks
```

The native Water HDA contract now verifies all 37 legacy macro
aliases against canonical hardware values; the CI job fetches the
canonical header and compatibility wrapper from the **same pinned
revision**, rather than building the entire Water toolchain.

The full HDA smoke additionally compiles
`tests/win95-hda-vxd-header.c` with Water's `vmm.h` and
`audio/hda_regs.h` into **i386 Windows COFF only**. This proves
the source can share relevant i386 types and header constants; it
does not link or load a VxD, verify Win95 services, or test HDA
hardware. `tests/win95-audio-win16-abi.c` separately checks the
new shared version alias with the **16-bit** OpenWatcom path.
The two checks are deliberately distinct: a Win16 `.DRV` cannot
be compiled as a 32-bit VxD merely because both can refer to HDA.

The Water update includes other upstream source changes beyond these
two small header additions. Review the pinned gitlink and validate
required tools in an existing checkout; **do not force a full Water
or LLVM bootstrap just to test an HDA register bitfield**. The project
never automatically rewrites submodule revisions.

The new HDA profile is included in `all` as a small compile-only
smoke check, but is never considered an installable Win95 driver.
The existing `WHPPROBE.EXE` DOS PCI BIOS inventory has also been
extended to *report* `04:03:00` HDA-class endpoints and flag
`1022:780D` as an AMD FCH candidate. It performs **no HDA MMIO
reads**. A DOS PCI listing, Windows 95 DOS prompt and real Windows
95 protected-mode enumeration are independent evidence, not
interchangeable runtime tests.

Next, gather the actual Pavilion PCI hardware tuple and an
independently authorized, read-only MMIO snapshot on a suitable
operating system. Only then evaluate codec link presence, codec
vendor/subsystem ID, nodes, amplifier/pin widgets and safe mixer
paths. A *working* Win95 RTM sound implementation also requires
an HDA bus/controller driver, appropriate VxD/Win16 multimedia
bridge, IRQ/DMA serialization and cache coherency, codec control,
correct speaker-jack routing, period-compatible DDK and loader
tests, and a reversible INF setup path. A DOS diagnostics target
cannot make HDA into a legacy Sound Blaster card; SB/FM emulation,
if pursued, is a separate compatibility project.

**Cost control:** Avoid rebuilding LLVM to test PCI/GCAP bitfields,
avoid hardware writes while identity is uncertain, and do not buy
or substitute a purported Realtek codec from a driver-download
listing. Keep the HDA read-only tests alongside the established
Windows 95 **Win16 audio DDK boundary** below without treating
the older SB16 .DRV ABI as a turnkey HDA implementation.

References:
- HP Pavilion p6-2310 sound specifications:
  https://www.newegg.com/hp-pavilion-p6-2310/p/N82E16883256196
- AMD FCH Azalia PCI ID comparison:
  https://cateee.net/lkddb/web-lkddb/SND_HDA.html
- Intel HD Audio controller specification:
  https://www.intel.la/content/dam/www/public/us/en/documents/product-specifications/high-definition-audio-specification.pdf
- PCI HDA class programming interface:
  https://pcisig.com/sites/default/files/files/PCI_Code-ID_r_1_12__v9_Jan_2020.pdf
- Realtek ALC655 identifies as AC'97:
  https://www.realtek.com/Product/Index?id=671
- Microsoft UAA audio architecture:
  https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/universal-audio-architecture

## Windows 95 RTM sound-driver DDK boundary

Windows 95 RTM audio cards use traditional Win16 multimedia installable
drivers (NE .DRV modules) for wave, MIDI, auxiliary and mixer messages,
potentially combined with separate protected-mode LE .VXD hardware drivers.
The 1995 DDK MSSB16.DEF Sound Blaster 16 sample includes DriverProc,
wodMessage, widMessage, auxMessage and mxdMessage with per-driver DEF
ordinals; those example ordinal numbers must not be hardcoded for other cards.

Water already has generic Win32 mmddk.h, mmsystem.h and WinMM/16-bit
MMSYSTEM compatibility code. Those headers are NOT authenticated Win95
RTM 16-bit driver headers, and Water's DLL imports are not sound-card
firmware or hardware DDK libraries. Instead of copying any proprietary
1995 DDK files, Water now owns a small opt-in Win16 audio include boundary:

    third_party/water/include/ddk/win95rtm/audio/ddk_audio.h

The base probe reuses Water's existing Win16 width/far-pointer checks.
The opt-in probe requires BOTH mmsystem.h and mmddk.h in an independently
verified historical DDK include directory; that path is searched before
OpenWatcom or generic Water headers. On case-sensitive hosts, normalize
file spelling separately from preserving original file provenance.

    ./build audio-win16-check
    WHP_WIN95_AUDIO_DDK_INCLUDE=/path/to/verified/win95/ddk/include ./build audio-ddk-check

These compile only Win16 objects with OpenWatcom wcc -bt=windows -mc
-bd -zu; they do NOT build/install a playable sound card driver, NE
.DRV file, VxD, INF, or WDM/NT .SYS. The historical DDK and local
toolchain have not been supplied or executed here, and successful object
compilation alone cannot verify the original binary ABI.

Before implementing physical sound I/O, identify a real Sound Blaster,
Crystal, ESS, etc. device by bus/PnP ID, chip revision, I/O base, IRQ,
DMA channels, codec/DSP and companion VxD. Keep existing Crystal MIDI
VxD evidence in Water documentation/cwbmidi-vxd.md; it primarily
covers later package releases and does not establish Win95 RTM support.

Historical sources:
- Windows 95 DDK audio overview: https://techshelps.github.io/MSDN/WIN95DDK/HTML/S2733.HTM
- Windows 95 DDK message reference: https://techshelps.github.io/MSDN/WIN95DDK/HTML/S2B24.HTM
- DDK MSSB16.DEF sample: https://techshelps.github.io/MSDN/WIN95DDK/HTML/S2AE1.HTM
- Microsoft KB Q152539: https://www.betaarchive.com/wiki/index.php/Microsoft_KB_Archive/152539
## Windows 95 RTM display-header boundary (Win16 versus VxD)

**Verified from the 1995 DDK display minidriver guidelines:** the display
minidriver's `PDEVICE` must match `DIBENGINE`, and a genuine driver needs
Enable, Disable, ReEnable, Control, and correct DIB Engine/VDD integration.
The related 32-bit mini-VDD uses separate VMM/VDD interfaces.

The Water-owned `third_party/water/include/ddk/win95rtm/` now has:
- `win16_abi.h`: shared Watcom **16-bit** compiler and segmented-pointer size
  assertions, using Watcom's own `windows.h`. It deliberately does not
  redefine historical `DIBENGINE`, `GDIINFO`, or the display exports.
- `ddk_minidrv.h`: **opt-in** inclusion of historical `gdidefs.h` and
  `dibeng.h` under `WHP_WIN95_DISPLAY_USE_HISTORICAL_DDK`.

The Windows 95 DDK documents the `MINIVDD.INC` assembly interface and
`DISPLAYINFO`. A separately recovered 9x C witness, VMDisp9x's
`ddk/minivdd.h`, documents VDD-call identifiers, dispatch indices and the
50-byte `DISPLAYINFO` field image. **That witness covers Windows 95/98/Me,
not exclusively 4.00.950.** WHP now provides a *limited, project-owned*
`include/minivdd.h` C compatibility shim backed by
`third_party/water/include/ddk/win95rtm/display/minivdd.h`. These expose a selected
cross-version-observed numeric surface and a checked field-image candidate,
not a verified RTM VDD callback ABI. The assembler `MINIVDD.INC` remains
separate and is **not** replaced or recreated.

**Header availability:** Water's generic Win32 headers are not a substitute
for the historical DDK. Only the version-scoped WHP reference headers were
moved to Water; `include/minivdd.h` remains a driver-facing compatibility
adapter, and Radeon-specific headers remain in `drivers/`.
OpenWatcom supplies generic Win16 `windows.h`. Historical `gdidefs.h` and
`dibeng.h` are still needed separately. The new `minivdd.h` is intentionally
a WHP-authored subset; no proprietary DDK file has been copied into WHP.
When a historical DDK is supplied, its include directory comes first so
the genuine header takes precedence over the WHP fallback.

```sh
./build.sh bootstrap-watcom
./build display-win16-check
./build minivdd-check
WHP_WIN95_DISPLAY_DDK_INCLUDE=/path/to/verified/win95/ddk/include \
    ./build display-ddk-check
```

The first check compiles a Win16 **object only** with OpenWatcom `wcc`,
`-bt=windows -mc -bd -zu`. The second requires an explicit DDK directory,
checks the two required headers, then compiles an opt-in inclusion probe.
It intentionally rejects missing or incompatible headers. Neither check
links a working Win16 `.DRV` nor proves Windows 95 RTM DIBENG or mini-VDD
calling conventions. A native macOS compiler run is still required before
marking these targets validated.

**Current remaining gaps:** verify the recovered `DISPLAYINFO` field image
against an original Windows 95 RTM `MINIVDD.INC`, including VDD dispatch
version limits and actual register conventions; verify full `DIBENGINE` binary layout;
`GDIINFO`/DDI prototypes and exported ordinals; special calling convention
for `CreateDIBPDevice`; 16-bit DIB Engine import libraries; separately
validated `MINIVDD.INC` VMM/VDD service ABI; a genuine NE `.DRV` linker
probe; disposable Windows 95 RTM VM testing. Keep the existing VGA fallback.

Evidence:
- Microsoft Windows 95 DDK, display minidriver guidelines:
  https://techshelps.github.io/MSDN/WIN95DDK/HTML/S27F2.HTM
- Microsoft Windows 95 DDK, refresh-rate VDD data `MINIVDD.INC`:
  https://techshelps.github.io/MSDN/WIN95DDK/HTML/S281C.HTM
- OpenWatcom Win16 16-bit compiler/memory models:
  https://open-watcom.github.io/open-watcom-v2-wikidocs/cpguide.html
- Michal Necasek, Win9x minidriver ABI investigation:
  https://www.os2museum.com/wp/windows-9x-video-minidriver-hd/
- JHRobotics 9x source, display header usage:
  https://github.com/JHRobotics/vmdisp9x/blob/main/enable.c
- JHRobotics cross-version C-header witness (not RTM provenance):
  https://github.com/JHRobotics/vmdisp9x/blob/main/ddk/minivdd.h

## Radeon HD 6410D (hardware target; PCI identity skeleton)

The first hardware target is the Radeon HD 6410D integrated GPU in the
WHP-researched HP Pavilion p6-2310 (AMD A4-3420). Earlier WHP observations
reported DVI output through the Windows 95 RTM generic `VGA.DRV` path;
this must remain the safe working display configuration until validated.

The Linux Radeon PCI table classifies `1002:9644` as a Sumo2 integrated GPU
and `1002:9645` as a Sumo2 mobility variant; the PCI ID database associates
both with the Radeon HD 6410D name. **The actual GPU device ID from the HP
machine remains unverified.** Other Sumo IDs must not be treated as this GPU.

`drivers/display/radeon_hd_6410d/` contains a C-only, *pure* PCI identity
classifier. It makes no VMM calls, does not enumerate a PCI bus, does not read
or write BARs, VRAM, MMIO, ROM, or firmware, and has no working Windows 95 driver to install. A **deliberately
non-installable** `R6410D.INF` now records the installation metadata shape,
but has no PCI binding or copy/registry directives. Its output is a
**candidate identification, never permission to bind**.

```sh
./build smoke-radeon6410d
```

The check uses the pinned LLVM compiler to produce an i386 Windows COFF
object, verifies its machine type, then runs independent native-host identity
tests. Passing does **not** mean a working Radeon display driver exists.

Before introducing any hardware interaction, capture from the real machine
(with read-only inventory tools): PCI vendor/device ID, revision, PCI class,
subsystem IDs, BAR types and ranges (without changing their configuration),
and the observed Windows 95 VGA fallback. Preserve the original driver and
boot settings. Verify that the PCI identity and firmware capabilities match.

### Windows 95 RTM Radeon INF (installation gate)

`packaging/win95rtm/R6410D.INF` is an ASCII Windows 95 research scaffold.
It includes the historical `[Version]` `Signature="$CHICAGO$"`, the
`Display` class, provider string, manufacturer section, and an **empty
models section**. The PCI candidate IDs `1002:9644` (desktop) and
`1002:9645` (mobility) appear **only in comments**. Neither device has
been independently confirmed from the user's actual HP hardware.

**DO NOT INSTALL OR COPY THIS INF INTO `WINDOWS\\INF`.** It is not a
driver package: no `.DRV`, `.VXD`, installer section, `CopyFiles`,
`AddReg`, automatic PCI ID match, monitor profile, DDC enablement, or
video-mode settings are present. These omissions are safety controls,
not missing defaults to guess. The previous `VGA.DRV` path remains
available. A real installable INF will only be produced after actual
hardware-id verification, functional Win16/mini-VDD binaries, and
disposable Win95 RTM guest tests; do not manually remove these guards.

```sh
./build inf-check          # requires only Python 3; no compiler/GPU
./build.sh inf-check       # same target through existing frontend
./build.sh test            # begins with INF safety checks
```

`tools/verify_radeon_inf.py` enforces the exact four-section skeleton,
prohibits additional active keys or sections, and confirms PCI IDs remain
comments. `tests/test_radeon_inf.py` uses negative tests for accidental
PCI binding, file copying, registry changes (including `DDC=1`),
post-Win95 directives, and duplicate keys. This is a **repo-side static
safety gate**, not a Windows Setup parser or Win95 acceptance test.

For later activation, compare historical Win95 display INF witness
`vmdisp9x.inf` (a modern cross-9x example, not RTM-specific) and
Microsoft's Windows 95 DDK INF documentation. In particular, `HKR,DEFAULT,drv`
and `HKR,DEFAULT,minivdd` must reference **real validated** driver files,
and `HKR,DEFAULT,DDC,,1` must not be added until the VDD/BIOS DDC
feature works. Do not substitute modern WDDM/NT `ClassGUID`, catalog,
or x64 INF conventions.

References:
- Historical Windows 95 DDK INF example:
  https://techshelps.github.io/MSDN/WIN95DDK/HTML/S2AD0.HTM
- Windows 95 DDK VESA DDC enablement requirements:
  https://techshelps.github.io/MSDN/WIN95DDK/HTML/S281E.HTM
- Windows 9x display INF cross-check (VMDisp9x):
  https://github.com/JHRobotics/vmdisp9x/blob/main/vmdisp9x.inf

### DVI/DDC evidence stage — read-only EDID parser

The Radeon identity skeleton now has a second pure-C component:
`drivers/display/radeon_hd_6410d/dvi_edid.[ch]`. It validates exactly
one **128-byte EDID 1.0–1.4 base block** supplied by a caller, checks the
8-byte header and checksum, records digital versus analog monitor input,
and decodes the first detailed timing candidate (pixel clock, active width
and height, horizontal/vertical blanking). It records advertised extension
count **without reading, trusting, or decoding extensions**. It accepts
a valid analog EDID without claiming a digital signal is available.

EDID is shared by VGA, DVI and other connectors: the `digital_input`
field **does not identify a DVI port** or distinguish DVI-D from DVI-I.
An ATOMBIOS connector-object path, board connector routing, DDC GPIO/I2C
pin mapping, optional hot-plug behavior, and encoder/transmitter mapping
still need read-only firmware/hardware verification. No EDID timing is
authorized for programming and no TMDS encoder/PLL writes are implemented.
Single-link/dual-link capabilities and monitor clock limits also remain
unknown. The existing Windows 95 `VGA.DRV` route remains untouched.

Run `./build smoke-radeon6410d` or select `radeon6410d` in
`./build.sh menuconfig`. The existing PCI smoke now also cross-compiles
the EDID parser as i386 COFF and runs **12 host-side synthetic regression
scenarios**. All fixtures are fabricated test inputs, not EDID captures
from the HP Pavilion or Radeon HD 6410D. This is not a working DVI driver.

Reference implementations for the interpretation and connector distinction:
- Linux DRM EDID format: https://gbmc.googlesource.com/linux/+/5e049663f678938dd56812ba870a471060f8ce28/include/drm/drm_edid.h
- Linux Radeon ATOMBIOS routing: https://kernel.googlesource.com/pub/scm/linux/kernel/git/will/linux/+/85d45adef06caa988506686527a5fedf856dc550/drivers/gpu/drm/radeon/radeon_atombios.c
- Linux Radeon ATOMBIOS connector definitions: https://code.googlesource.com/linux/torvalds/linux/+/826c484166f0f74bd8fc09220f99dc937c9297cf/drivers/gpu/drm/radeon/atombios.h

Windows 95 RTM requires a Win16 display minidriver (`.DRV`) integrated with
the DIB engine, plus appropriate VDD/mini-VDD (`.VXD`) support where needed.
Do not use modern WDDM or NT display-miniport headers as substitutes.
The Win16, LE/VMM, framebuffer, modesetting, acceleration, and safe guest
tests are **all pending**. No AMD GPU registers are assumed or initialized.

Hardware/reference sources:
- Linux Radeon Sumo2 device table:
  https://lists.openwall.net/linux-kernel/2026/04/30/866
- PCI IDs (historical pinned snapshot):
  https://kernel.googlesource.com/pub/scm/utils/pciutils/pciutils/+/a9df1d1baccbafcd1f4bd7622dd20d3ea684fb75/pci.ids
- Windows 95 DDK, display minidriver guidelines:
  https://techshelps.github.io/MSDN/WIN95DDK/HTML/S27F2.HTM
