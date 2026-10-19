# WHP-Drivers

WHP hardware-driver research and development.

## Configurable build entry point

Use `./build.sh` as the new top-level entry point; the historical executable
`./build` remains intact as a backend and preserves all existing commands.

```sh
./build.sh menuconfig         # interactive portable shell menu, save with 's'
./build.sh config             # inspect active options
./build.sh build              # run configured smoke profile (default: ABI)
./build.sh test               # LE/format tests plus configured profile
./build.sh test-menuconfig    # isolated shell-menu regression checks
./build.sh bootstrap          # explicitly compile the pinned LLVM fork
./build.sh bootstrap-watcom   # bootstrap pinned OpenWatcom host + release tools
./build.sh bootstrap-all      # build both pinned toolchains
```

The menu selects one workflow (`abi`, `vxd`, `radeon6410d`, or `all`),
the VxD backend (`llvm` or optional `openwatcom`), and **independent**
toolchain bootstrap (`off`, `llvm`, `openwatcom`, or `both`), plus LLVM
parallel jobs (1–16, default 2). Earlier `WHP_DRIVER_BOOTSTRAP=on` profiles
are accepted and interpreted as `llvm`; `off` remains the default.
The Radeon identity profile remains LLVM-based; the backend choice affects
**only the VxD smoke probe**. OpenWatcom is never a silent fallback, and
the VxD backend must pass its own LE verification. OpenWatcom bootstrap
is independent of whichever backend runs the VxD probe.

Settings are stored in ignored `.whpconfig` as validated data (not
shell-sourced); environment overrides are `WHP_DRIVER_PROFILE`,
`WHP_DRIVER_VXD_BACKEND`, `WHP_DRIVER_BOOTSTRAP`, and `WHP_LLVM_JOBS`.
The front end performs **no automatic Git updates**, submodule pin changes,
driver installation, or GPU access. On an existing checkout, `./build`
still works exactly as before. A fresh checkout needs the pinned LLVM
submodule and a built LLVM toolchain before ordinary smoke commands work
unless LLVM bootstrap is explicitly selected. This menu is a portable
text interface, not a curses dependency.


## Pinned Water source (Win32 headers and import-library research)

The existing [Water fork](https://github.com/retrochristian5000/water) is pinned
at `third_party/water` as a separate Git submodule. To initialize only this
source dependency in an existing checkout:

```sh
git submodule update --init third_party/water
```

Its `include/` tree is available for **selectively audited Win32 user-mode
helper headers**. Water's import libraries are build outputs, not assumed to be
present in the source checkout or interchangeable with Windows 95 RTM libraries.
No Water runtime or library is added to the VxD link by default.
A minimal, i386-only Windows 95 DDB/control-message baseline now lives in
`third_party/water/include/whp/win95rtm/vmm.h` and is compiled by
`./build smoke-vxd`. All shared WHP Windows 95 RTM headers are owned by
Water and consumed at the recorded gitlink revision. Header probes fail
if the Water submodule is absent or checked out at a different revision.
The header is **not** a complete VMM DDK: live service calls, the control
ABI, and LE service linking remain unverified work.
Use the recorded gitlink revision (`20015a3fa88b`), not the moving Water `master` branch.

## OpenWatcom fallback candidate (explicit, never automatic)

WHP-Drivers pins the existing [WHP OpenWatcom fork](https://github.com/retrochristian5000/openwatcomfork)
as a separate Git submodule at `toolchains/openwatcom`. Its commit is fixed
in the Git tree; do not substitute an unverified moving upstream tip.

```sh
git submodule update --init toolchains/openwatcom
```

The fork includes Darwin bootstrap logic and macOS Intel/ARM64 CI build
configurations. These show **source support**, not a successful build on every
Mac. Use the fork's OpenWatcom build guide to create its `rel` release tree.
For native tools, macOS ARM64 uses `rel/armo64` and macOS Intel uses
`rel/bino64`. A source checkout alone has no runnable compiler.

```sh
./build watcom-check       # pinned source + native tool presence only
./build watcom-smoke-vxd   # independent compiler/linker and LE format probe
```

The optional environment variables `WHP_WATCOM_ROOT` and
`WHP_WATCOM_BIN_DIR` can select an explicitly prepared release tree and
host-binary directory. By default the tests use `toolchains/openwatcom/rel`.
An external install is not automatically guaranteed to match the pinned fork.

**Do not enable OpenWatcom as the default VxD linker.** Its documented
`FORMAT WINDOWS VXD DYNAMIC` path has an [upstream regression report](https://github.com/open-watcom/open-watcom-v2/issues/310)
for NE rather than LE output; later linker changes in the fork have not yet
been validated here. The opt-in probe compiles using Watcom's own x86
object format, attempts a dynamic VxD link, and runs our existing independent
LE verifier. A missing tool, link failure, NE/PE output, or malformed LE
is a hard failure, **never** a successful fallback. Even a passing structural
probe is *not* permission to load the result in a Windows 95 RTM guest.

LLVM remains the default toolchain. Neither OpenWatcom nor Water adds
implicit runtime libraries to VxD links.

### Bootstrapping OpenWatcom independently

The OpenWatcom build uses the pinned fork's CI stages directly:
`ci/buildx.sh` with `OWBUILD_STAGE=boot` (native `wmake` and `builder`),
then `OWBUILD_STAGE=build` (native compiler, assembler and linker release
artifacts under `rel/`). Both stages must succeed, followed by the existing
`./build watcom-check` check for `wcc386` and `wlink`.

```sh
./build.sh bootstrap-watcom   # OpenWatcom alone, no LLVM bootstrap required
./build.sh bootstrap-all      # LLVM first, then OpenWatcom
./build.sh menuconfig         # choose off / llvm / openwatcom / both
```

The bootstrap invokes `OWTOOLS=CLANG` without sourcing `setvars.sh`, which
would otherwise override the selection with GCC. It reuses a previously
built pinned WHP LLVM Clang when available, or announces a fallback to
host Clang. Set `WHP_WATCOM_CLANG` to choose a specific host compiler.
No automatic network fetching, revision changes, or driver installation is
performed. The fork's scripts have limitations with spaces and `=` in
source paths. macOS native build dependencies (potentially DOSBox-X) still
need to be installed independently; actual macOS end-to-end completion
is not yet verified.

## Windows 95 RTM display-header boundary (Win16 versus VxD)

**Verified from the 1995 DDK display minidriver guidelines:** the display
minidriver's `PDEVICE` must match `DIBENGINE`, and a genuine driver needs
Enable, Disable, ReEnable, Control, and correct DIB Engine/VDD integration.
The related 32-bit mini-VDD uses separate VMM/VDD interfaces.

The Water-owned `third_party/water/include/whp/win95rtm/display/` now has:
- `win16_abi.h`: Watcom **16-bit** compiler and segmented-pointer size
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
`third_party/water/include/whp/win95rtm/display/minivdd.h`. These expose a selected
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

## LLVM toolchain

This repository pins the existing [WHP LLVM fork](https://github.com/retrochristian5000/LLVM)
as a Git submodule at `toolchains/llvm-project`, following the WHP/QEMU convention.
The pinned revision is recorded in Git (not the moving tip of `main`).

```sh
git clone --recurse-submodules https://github.com/retrochristian5000/WHP-Drivers.git
cd WHP-Drivers
./build bootstrap
./build smoke
```

For an existing clone, initialize the pinned source once:

```sh
git submodule update --init toolchains/llvm-project
```

`./build bootstrap` uses host CMake and Ninja to build a small native compiler
suite from this fork (Clang, LLD, and LLVM binary utilities). By default it uses
`.build/llvm-native` and up to two parallel jobs. Set `WHP_LLVM_JOBS` to change
the job count, or `WHP_LLVM_BUILD_DIR` to move the build directory.
To reuse an **already built copy of this same fork**, set `WHP_LLVM_BIN_DIR`
to its `bin` directory and run `./build check` or `./build smoke`.
Neither check silently selects Apple's or the system's Clang.

`./build check` validates the pinned source checkout and tool executables.
`./build smoke` then compiles a **freestanding i386 Windows COFF object**,
checks its machine type, and leaves it at
`.build/llvm-native/whp-tests/i386-abi-probe.obj` (or the selected build directory).
It is a compiler/ABI smoke test, **not a Windows driver**.

The bootstrap compiler itself remains native to the development host. The
Windows x86 target triple is applied only when compiling the smoke-test object;
do not propagate i386 flags into host CMake, Ninja, or LLVM's own build.

## Windows 95 VxD linking: NOT YET ENABLED

Windows 95 RTM VxDs need a **genuine LE (Linear Executable)** image, historical
VMM/VxD interfaces, appropriate LE relocations, and the driver descriptor
block (DDB) as export ordinal 1. Clang can generate x86 COFF objects, but
`lld-link` emits PE/COFF images, not LE VxDs. Never rename a PE file `.VXD`
or pass a successful COFF compilation off as an LE linker test.

Microsoft's Windows 95 DDK documents a historical `LINK.EXE /VXD` route.
Open Watcom's documented `FORMAT WINDOWS VXD` path has an outstanding
reported failure to emit proper LE images (open-watcom-v2 issue #310).
Do **not** silently select either linker without its own verified test.

The deliberately separate format preflight runs on an **existing** file:

```sh
./build test-format
./build verify-vxd /path/to/known-driver.vxd
```

`test-format` runs synthetic structural regression tests (no driver binaries
are produced). `verify-vxd` rejects PE/NE/LX, missing ordinal-1 exports,
out-of-range object/page tables, and malformed or unsupported internal fixups.
It currently supports direct-header dynamically-loadable LE VxDs; embedded
LE resources and static-VxD variants are not yet covered.

**Passing format preflight is not permission to ship or install a driver.**
Before enabling VxD builds, prove that an actual linker emits an LE image with
the correct VxD control/DDB references, test it in a disposable Windows 95
RTM VM, and independently validate relocations and initialization. The LLVM
fork's own LE writer is a separate, still-open implementation task.
NT `.SYS`, WDM, and Win9x VxD contracts remain distinct.

Sources:
- Windows 95 DDK, *Building a Virtual Device*:
  https://techshelps.github.io/MSDN/WIN95DDK/HTML/S2305.HTM
- Geoff Chappell, *Dynamic VxD Loading* (WfW 3.11 loader analysis;
  version-specific and not a universal Windows 95 loader specification):
  https://geoffchappell.com/notes/windows/archive/vxdldr.htm
- Open Watcom VxD output problem: https://github.com/open-watcom/open-watcom-v2/issues/310

## Experimental WHP LE writer (format smoke only)

The pinned LLVM fork now contains a first-stage **standalone Python**
i386 COFF-to-LE writer under lld/utils/whp-le-link.py. This is **not**
native LLD PE/COFF output and is not yet proved to load in Windows 95.

After bootstrapping the compiler from the pinned WHP LLVM fork:

    ./build test-le
    ./build smoke-vxd

The first runs writer regression tests (no native compiler needed); the second
uses our Clang to compile an 80-byte-DDB structural probe, invokes the
experimental LE writer, and runs the independent WHP-Drivers LE verifier.
The result is saved under .build/llvm-native/whp-tests/WHPTEST.VXD.

The probe deliberately lacks a production-ready VMM driver contract. A
successful structural test is not permission to load on real hardware.
Guest loading, DDB control dispatch, and VMM service linking must be tested
independently before driver development is unlocked. The current LE writer
rejects unsupported COFF formats and relocations instead of silently emitting
a malformed image.
