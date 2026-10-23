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
`third_party/water/include/ddk/win95rtm/vmm.h` and is compiled by
`./build smoke-vxd`. All shared WHP Windows 95 RTM headers are owned by
Water and consumed at the recorded gitlink revision. Header probes fail
if the Water submodule is absent or checked out at a different revision.
The header is **not** a complete VMM DDK: live service calls, the control
ABI, and LE service linking remain unverified work.
Use the recorded gitlink revision (`bdd44d06ee26`), not the moving Water `master` branch.

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

## Windows 95 hotfixes as ABI and negative-regression evidence

The pinned Water ledger `documentation/win95rtm-hotfix-regressions.md`
connects Microsoft Q256015 / MS00-017 (March 2000 DOS device-name
path crash, fixed by release-specific IFSMGR.VXD binaries), Q138250
(1995 FAT 8.3 tilde alias classification), and Q161020 (Windows 95
updates installed by W95upd.inf via SETUPX.DLL or ADVPACK.DLL).

Track Windows 95 RTM 4.00.950, retail IFSMGR 4.00.951 / 4.00.954,
OSR2 IFSMGR 4.00.1116, and Windows 98/98 SE variants separately.
The same filename does NOT confer compatibility across releases.

Water's current `dlls/ifsmgr.vxd/ifsmgr.c` handles selected
`DeviceIoControl` requests but does not implement the native 9x
IFSMGR pathname dispatch. Do not patch the unrelated modern
`dlls/ntdll/path.c` merely to imitate IFSMGR; distinguish safe
path-component parsing from disk/network redirector access.

Keep tests synthetic and side-effect-free until the authentic
ring-0 dispatch path and patch binary are independently verified.
Never execute a known crash-inducing path on an unpatched Windows 95
host or install an arbitrary archived hotfix on a real machine.
### Windows 95 RLE graphics evidence and VfW decompression guard

Water now audits the Windows 3.x/95-era DIB RLE4/RLE8 format
separately from Video for Windows MSRLE compression.
`dlls/msrle32/msrle32.c` preflights compressed packet lengths,
word padding, run widths, vertical deltas and an end-of-image
marker before invoking the existing MSRLE codec decoder. A synthetic
C99 source harness is in `scripts/tests/test_msrle_stream.py`.
This is a defensive compatibility change, not proof of exact
Win95 RTM codec error semantics.
The MSRLE32 encoder also now correctly extracts all MSB-first
1bpp input pixels, including partial final bytes (synthetic source
regression in `scripts/tests/test_msrle_1bpp.py`).

The GDI path (`dlls/win32u/dib.c`) and later WIC decoder
(`dlls/windowscodecs/bmpdecode.c`) remain distinct and unmodified.
RLE bitmap decoding is not disk compression, DBLBUFF, IOS, or
INF installation. See Water's
`documentation/win95-rle-graphics.md` for the evidence and
remaining palette/stride/runtime test gaps.

### INT13.VXD versus IOS and BIOS disk emulation

The pinned Water revision now includes a separate `dlls/int13.vxd`
compatibility boundary and routes the historical Windows 95 VxD device
ID `0020h` independently from IOS `0010h`. The new module explicitly
rejects unsupported service/IOCTL calls and performs no host disk I/O.

It must NOT import `programs/ntio.sys/int13.c` from Water's DOS/NTVDM
compatibility path: its BIOS drive `80h` emulation currently maps to
a host **logical volume**, not a physical disk MBR at LBA 0. Mixing
those coordinates could read or overwrite unintended sectors.
See `third_party/water/documentation/win95rtm-int13-vxd.md`.

Historical Win95 RTM DDB/interrupt hooks, VMM32 startup and IOS
handoff are still not implemented. Do not package this placeholder as
a working original Windows 95 INT13 driver. No driver binding or
installable Radeon INF was enabled by these changes.

### AMD K6-2 high-speed CPU startup update (OSR2; Q192841/Q234259)

The Water hotfix ledger now records `AMDK6UPD.EXE`: the high-speed
CPU divide exceptions Microsoft documented in `IOS.VXD`,
`ESDI_506.PDR` and `SCSIPORT.PDR`. The official fix is for Win95
**OSR2/2.1/2.5**, not retail Win95 RTM. Preserve this release split;
the OSR2 PDRs are not NT `scsiport.sys` or generic Windows 95 drivers.

The update was revised: the original `VFBackup.VXD` 4.00.1112
was superseded by 4.00.1114 after backup failures, with QFE
metadata `980911N1` versus `990602N1`. Inspect package members,
file-version resources and their hashes before deriving binary
behavior; unchanged byte size does not imply unchanged code.

Current Water `dlls/ios.vxd/ios.c` is a partial service compatibility
layer and does not have the original timing calibration, IDE or SCSI
driver initializers. Do not implement a hardcoded 350 MHz cutoff,
fabricate successful IOS service calls or import OSR2 code into RTM.
Microsoft Q312108's 2.2 GHz NDIS divide error is a *separate* bug.

Full evidence and synthetic test matrix:
`third_party/water/documentation/win95rtm-hotfix-regressions.md`.
## Windows 95 RTM Add New Hardware installation boundary

### Win16 SETUPX ordinal 312: guarded preselection

The pinned Water `dlls/setupx.dll16` now maps `DiSelectDevice` ordinal
312 to a limited, no-UI `DiSelectDevice16` implementation. It checks
`DEVICE_INFO16` size and recognizes a caller's non-NULL, already
selected compatible/class driver list **head**; otherwise it returns
an explicit SETUPX error with no installation side effects.
This pointer relationship does NOT prove an INF is valid, hardware is
supported, or a driver was installed. No original Win95 RTM guest ABI
or authentic wizard UI has yet been tested. A standalone C harness is
in Water at `scripts/tests/test_setupx_select_device.py`.

**Keep `R6410D.INF` non-installable.** No hardware ID binding, file
copy, registry change, class installer, or Radeon mode-setting was
added by the SETUPX improvement. Driver-list generation and authentic
manual selection/Have Disk interactions remain unresolved.

Historical Windows 95 Control Panel offers **Add New Hardware** with
automatic detection and a manual category -> manufacturer -> model ->
Have Disk route. A new Plug and Play device can also trigger a separate
automatic installation/media-prompt path at boot or insertion.
Microsoft Q142984 documents OEMSETUP.INF for manufacturer drivers.

These are user-interface and setup mechanisms, not evidence that a
chosen driver matches actual hardware or that installation succeeded.
Legacy ISA devices may need manual I/O port, IRQ, DMA and memory resource
allocation; Plug and Play devices are enumerated and resource-arbitrated.
Sound Win16 .DRV, 9x VxDs, NDIS 3.1 miniports and display mini-VDDs
retain their independent ABI and driver-service requirements.

The pinned Water ledger `documentation/win95rtm-add-new-hardware.md`
audits Windows 95's SETUPX-related workflow against Water source.
Water already owns `dlls/setupx.dll16` and generic INF parsing, but
the driver-selection, Have Disk and installation functions needed
for an authentic wizard are not yet implemented. Its newer SetupAPI/
Configuration Manager/Newdev code is NOT proof of RTM wizard behavior.
Windows 95 retail and OSR2 also differ in documented bad-INF symptoms.

**Safety invariant:** `packaging/win95rtm/R6410D.INF` remains strictly
NON-INSTALLABLE. Its empty model list must NOT be filled simply to
show the Radeon in a wizard. `./build inf-check` remains the preflight
for that scaffold. Do not copy research INFs to WINDOWS\\INF or change
real PnP resource settings. Inspect installation changes only in a
disposable RTM guest with original driver media and snapshots.

References:
- Win95 Installing New Devices: https://techshelps.github.io/MSDN/DNWIN95/HTML/S64D5.HTM
- Microsoft Q142984: https://helparchive.huntertur.net/document/43903
- Microsoft Q182723 retail vs OSR2: https://techshelps.github.io/MSDN/KB/Source/win95x/q182723.htm
## Windows 95 RTM Ethernet DDK (NDIS 3.1)

### RTL8111E header dependency audit (2026-10-23)

WHP-Drivers pins Water commit `bdd44d06ee26ee4ab0ccfd880646a5c72907aa4c`.
At that revision, Water's `include/ddk/win95rtm/network/ndis31.h`
has only a 32-bit x86/opt-in include boundary. It does not supply
the NT 3.51 NDIS miniport ABI, callbacks or import library.
Water's `include/ddk/ndis.h` defines protocol/CoNDIS-era types and
does **not** define the classic `NDIS_MINIPORT_CHARACTERISTICS`
required by this RTL8111E work. `include/ntddndis.h` contains
Ethernet OIDs but also newer WLAN and PnP definitions; presence of
individual OIDs is **not** proof of NT 3.51 DDK ABI compatibility.

The exact missing or unverified pieces for an installable miniport are:
- Historical `NDIS_MINIPORT_CHARACTERISTICS`, `NDIS_PACKET`,
  `NDIS_BUFFER`, `NDIS_HANDLE`, `NDIS_STATUS` layouts and callbacks
  with NT 3.51 calling conventions.
- Historical `NdisMInitializeWrapper`, `NdisMRegisterMiniport`,
  `NdisMSetAttributes`, IRQ/DMA/resource APIs, and the actual Windows
  95 NDIS wrapper import/link and loader ABI.
- NDIS 3.x Ethernet OID/filter handling, network media status and
  packet send/receive completion semantics.
- Independently verified NT 3.51 DDK headers/libs, the Pavilion's
  PCIe/bus enumeration and driver binding evidence.

The RTL8111E chip register definitions belong in
`drivers/network/rtl8111e/`, not in Water's generic NDIS DDK
interface. Similarly, Water's newer NDIS 6.0
`NDIS_MINIPORT_DRIVER_CHARACTERISTICS` and NetBufferLists interfaces
must **not** be used for the Windows 95 RTM driver.

The existing `./build ndis31-ddk-check` opt-in probe now checks a
larger *declaration surface*: miniport version fields, packet type,
Ethernet medium and OIDs, legacy registration entry points, and
32-bit fundamental NDIS widths. It deliberately cannot run without
a separately verified NT 3.51 DDK include directory; it cannot
prove linking, DMA, operation, or Windows 95 loading. No Water
submodule revision changed during this audit.

Sources: Microsoft 1995 Windows 95 DDK:
https://techshelps.github.io/MSDN/WIN95DDK/HTML/S2D14.HTM
and https://techshelps.github.io/MSDN/WIN95DDK/HTML/S2D16.HTM

### RTL8111E NT 3.51 miniport callback adapter (2026-10-23)

`drivers/network/rtl8111e/ndis31_adapter.c` now contains a **DDK-backed
source-level adapter** for the legacy NDIS 3.0 callback shape used by
NT 3.51 miniports and the Windows 95 NDIS 3.1 Plug and Play wrapper.
It assigns Initialize, Halt, Reset, QueryInformation, SetInformation,
Send and TransferData handlers directly into the historical
`NDIS_MINIPORT_CHARACTERISTICS` fields, without casting mismatched
function pointers or reconstructing the structure in Water. The code
uses `NdisMInitializeWrapper`, `NdisMRegisterMiniport`, and
`NdisTerminateWrapper` in a named **registration probe**, deliberately
not an exported `DriverEntry`.

The adapter intentionally returns `NDIS_STATUS_FAILURE` on hardware
initialization and all data-plane operations. It does **not** advertise
a usable RTL8111E, select PCI BARs, enable interrupts, start DMA or
pretend that the lifecycle core is already bound to NDIS. The probe's
register-success path does not create an installable driver. The
existing `win95-ndis31-header.c` type probe no longer takes the address
of `NdisMInitializeWrapper`: some legacy headers define that identifier
as a **function-like macro** and so `sizeof(&NdisMInitializeWrapper)`
was an invalid declaration test. The registration probe instead
exercises the actual call syntax at compile time.

```sh
# No DDK needed: four synthetic-header tests (not authenticity tests).
./build test-ndis31-adapter

# Real historical header compile: includes existing DDK provenance checks.
WHP_WIN95_NDIS31_DDK_INCLUDE=/verified/nt351/inc \
WHP_NT351_DDK_NDIS_SHA256=REPLACE_WITH_VERIFIED_NDIS_SHA256 \
WHP_NT351_DDK_NTDDK_SHA256=REPLACE_WITH_VERIFIED_NTDDK_SHA256 \
./build ndis31-adapter-check
```

A synthetic 3.x header fixture and mocked wrapper verify callback
failure paths, cleanup on rejected registration, architecture-targeted
i386 COFF compilation, and refusal to compile without the DDK guards.
This fixture **is not the NT 3.51 DDK**; its callback definitions only
support local tests. The DDK compile target uses the pinned Water
boundary, a supplied historical include tree and the existing pinned
OpenWatcom compiler. Its object format and compatibility with
historical `ndis.lib` still require explicit link verification.

The remaining implementation gap is **real hardware enumeration,
resource ownership, DMA/interrupt serialization, Ethernet OIDs and
packet indications**. This object must not be deployed to Windows 95.

Sources: Microsoft Windows 95 DDK
https://techshelps.github.io/MSDN/WIN95DDK/HTML/S2D14.HTM ;
NDIS 3.0 structure in historical-style public-domain w32api
https://chromium.googlesource.com/external/github.com/zephyrproject-rtos/picolibc/+/7cadc91d9843048a161fea8469ffc2508b226a5b/winsup/w32api/include/ddk/ndis.h
(comparison only; not authenticated NT 3.51 DDK).

### NDIS 3.1 link-input research gate (2026-10-23)

A Win95 RTM NDIS miniport is **not** a full-MAC LE VxD. Our existing
`smoke-vxd` and LE writer are not the linker recipe for the RTL8111E.
The miniport's platform-facing object must be compiled using a genuine,
verified NT 3.51 DDK contract, and its import names must resolve to
historically compatible exports. A `.SYS` extension is not a proof of a
loadable Win95 RTM miniport.

**Candidate library families, not automatic linker inputs:**

- `ndis.lib`: NDIS wrapper import candidate, expected for calls to
  `NdisM*` and `Ndis*`. Must come from the authenticated historical kit.
- `ntoskrnl.lib`: add **only** if validated NT 3.51-era imports are
  genuinely referenced and compatible with Win95's miniport wrapper.
- `hal.lib`: similarly conditional; do not bypass NDIS abstractions
  by importing arbitrary NT HAL functions.
- Compiler helpers: inspect each emitted undefined symbol and provenance.
  Never silently pull host C runtime, Win32 DLL imports, modern CRT,
  WDM or unrelated VxD service libraries into the miniport.

New read-only commands:

```sh
./build ndis31-link-inventory --lib /verified/nt351/lib/i386/ndis.lib
./build ndis31-link-audit \
  --lib /verified/nt351/lib/i386/ndis.lib \
  --pin ndis.lib=REPLACE_WITH_VERIFIED_64_DIGIT_SHA256 \
  --obj /verified/rtl8111e-ndis31-adapter.obj \
  --nm /verified/whp-llvm/bin/llvm-nm
./build test-ndis31-libs
```

The first command reports the archive magic, file name and SHA256 without
linking; the second accepts explicit, independently pinned input archives
and standard i386 COFF objects, uses the specified `llvm-nm` to compare
**exact** decorated undefined/defined names, reports unresolved symbols,
and prints a **proposed input order**, not linker flags. It deliberately
rejects an empty miniport object list or one without any unresolved
`Ndis*` calls: our current RTL8111E identity and miniport lifecycle
objects contain no NDIS registration adapter and cannot form a real
miniport image yet.

**Limits:** an `!<arch>` archive header is only a format preflight;
it is NOT validation that every member is i386 COFF or that an archive
has authentic DDK provenance. Name resolution is not proof of ABI,
stdcall stack widths, relocations, linkage, runtime exports, compiler
helpers, or correct PE image flags. The exact NT 3.51 DDK link
recipe, the needed compiler support archives and Windows 95 loader
behavior remain **unverified**; never ship or bind based on this output.

Evidence:
- Microsoft 1995 DDK, Windows 95 miniport recommendation:
  https://techshelps.github.io/MSDN/WIN95DDK/HTML/S2D14.HTM
- Microsoft 1995 DDK, miniports vs full-MAC VxD driver names:
  https://techshelps.github.io/MSDN/WIN95DDK/HTML/S2D16.HTM
- Novell Technical Information, November 1996, miniport `*.SYS`
  against Windows 95 `NDIS.VXD`:
  https://support.novell.com/techcenter/articles/ana19961101.html

### DDK-provenance and miniport build gate (2026-10-23)

The miniport route requires an independently verified **Windows NT 3.51 DDK**,
particularly the `ndis.h` / `ntddk.h` declaration set and the original
network-driver build/link examples and libraries. The **Windows 95 DDK** is
a separate source for RTM network PnP and INF installation behavior; its
`ndis.mk` extension guidance refers to **full MAC VxDs, not NT-compatible
miniport binaries**. Do not direct our experimental LE VxD writer at the
RTL8111E miniport or assume renaming a `.SYS` file creates a `.VXD`.

Confirmed sources:
- Microsoft, *About Windows 95 Network Device Drivers* (1995):
  https://techshelps.github.io/MSDN/WIN95DDK/HTML/S2D14.HTM
- Microsoft, *Windows 95 and Windows for Workgroups* (1995):
  https://techshelps.github.io/MSDN/WIN95DDK/HTML/S2D16.HTM
- Microsoft, *Where can I find more information on the Windows 95 network
  architecture?*: https://techshelps.github.io/MSDN/WIN95DDK/HTML/S2D04.HTM
  (NT 3.51 DDK, Network Drivers, Design Guide, Part 2, Chapter 7
  **Miniport NIC Driver Operations** and Chapter 15 **Full NIC Driver
  Operations**)
- Microsoft NDIS versions matrix, for version exclusion (not legacy ABI
  header provenance):
  https://learn.microsoft.com/en-us/windows-hardware/drivers/network/overview-of-ndis-versions

**Version constraints:** Win95 RTM = NDIS 3.1; Win95 OSR2's NDIS 4.0
includes `MiniportSendPackets`, so that callback is NOT a baseline RTM
entry point. Do not use NDIS 6 `NdisMRegisterMiniportDriver`,
`NDIS_MINIPORT_DRIVER_CHARACTERISTICS`, `MiniportInitializeEx`,
`NET_BUFFER_LIST` or the WDM resource model for this driver.

Current outstanding DDK-dependent requirements:
1. Exact legacy `NDIS_MINIPORT_CHARACTERISTICS` field order and sizes,
   pointer conventions, `DriverEntry`, `NdisMInitializeWrapper`,
   `NdisMRegisterMiniport` and adapter attribute setup.
2. Miniport initialization, halt, reset, query/set-information, send,
   transfer-data, ISR/interrupt-handler and check-for-hang signatures,
   required-vs-optional callbacks, concurrency/IRQL requirements and
   completion/receive-indication contracts.
3. NDIS 3.x Ethernet OID table, status codes, packet/buffer chaining,
   resource and DMA allocation/mapping and wrapper import symbols.
4. Historical miniport linker recipe, startup semantics, binary format,
   Win95 RTM INF/PnP binding, and actual guest loading.
5. Real RTL8111E PCIe enumeration and resource access on the Pavilion;
   the portable miniport lifecycle core does not solve this.

**Preflight added:** `./build ndis31-ddk-inventory` runs without LLVM
or OpenWatcom and reports fingerprints for candidate `ndis.h` and
`ntddk.h` (read-only). `./build ndis31-ddk-check` now requires **both**
SHA256 pins and rejects changed hashes or selected later-generation NDIS
identifiers before running the existing DDK-backed compile probe.
The lightweight identifier scan covers the two named headers only: it
cannot verify transitive includes, callback binary layout, imports,
license, vendor authenticity or historical kit provenance. Hash pins
prove the **same bytes** were supplied, not that those bytes came from
Microsoft's original DDK. The compiler probe still requires the pinned
OpenWatcom tree and a user-supplied historical DDK.

```sh
# Read-only candidate inspection: not authentication.
WHP_WIN95_NDIS31_DDK_INCLUDE=/verified/nt351/inc ./build ndis31-ddk-inventory

# Record hashes only after independently verifying the original kit.
WHP_WIN95_NDIS31_DDK_INCLUDE=/verified/nt351/inc \
WHP_NT351_DDK_NDIS_SHA256=REPLACE_WITH_VERIFIED_NDIS_SHA256 \
WHP_NT351_DDK_NTDDK_SHA256=REPLACE_WITH_VERIFIED_NTDDK_SHA256 \
./build ndis31-ddk-check

# Synthetic fixture regressions; does not require or reproduce a DDK.
./build test-ndis31-audit
```

**No historical DDK was supplied in this round:** no successful DDK ABI
compilation, miniport linking, Win95 loading, INF activation or Ethernet
operations are claimed. Water's revision remains pinned and unchanged.

Water moved its project-authored Windows 95 RTM reference headers from
`include/whp/win95rtm/` to `include/ddk/win95rtm/`, a platform/release
directory under the conventional DDK tree. Existing `WHP_` constants
remain namespaced because they are not proven exact Microsoft DDK declarations.
The Win16 width/far-pointer checker is now shared at
`include/ddk/win95rtm/win16_abi.h`, not duplicated in audio/display.

Windows 95 RTM Ethernet NIC drivers use NDIS 3.1. Microsoft's 1995
network driver DDK recommends **NT 3.51 DDK** miniport headers for binary
compatibility across Windows 95 and NT 3.51. The older NDIS 3.0 full MAC
model can also be recompiled with Win95 DDK tools for `.VXD` output.
An NDIS miniport and a full MAC driver are different binding contracts;
do not confuse either with DOS NDIS 2, contemporary NT `.SYS` runtime
libraries, or a modern NDIS 6 driver.

Water's general `include/ddk/ndis.h` includes later API concepts and is
**not** authenticated as the NT3.51/Windows95 NDIS header. The new
`include/ddk/win95rtm/network/ndis31.h` therefore exports no guessed
interfaces by default and requires verified DDK headers on opt-in.

    WHP_WIN95_NDIS31_DDK_INCLUDE=/path/to/verified/nt351/ddk/include ./build ndis31-ddk-check

This runs OpenWatcom `wcc386` in a **compile-only** 32-bit NT-target
configuration using historical `ndis.h` and `ntddk.h` first in the
include path. `-bt=nt` in this header probe does NOT mean the resulting
object is a Windows 95 loadable LE `.VXD` or a valid NT `.SYS` driver.
No NIC was selected, no real hardware or network was accessed, no INF
was installed, and no packet send/receive or miniport initialization
functionality has been invented. Header selection and actual ABI layouts
still require a provenance-checked DDK and Windows 95 guest tests.

Source trail:
- Microsoft Win95 DDK network model: https://techshelps.github.io/MSDN/WIN95DDK/HTML/S2D14.HTM
- Windows 95 versus Windows for Workgroups NDIS: https://techshelps.github.io/MSDN/WIN95DDK/HTML/S2D16.HTM
- NT 3.51 DDK network design: https://techshelps.github.io/MSDN/WIN95DDK/HTML/S2D04.HTM
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

## Realtek RTL8111E (HP Pavilion p6-2310; staged miniport development)

The HP Pavilion p6-2310 (H3Y76AA) has a reported Realtek RTL8111E
PCI Express Gigabit Ethernet controller. Prior WHP LISTPCI observations
found the controller at PCI bus 003, device 00, while Windows 95 RTM
Device Manager did not expose it. Windows 95 RTM `PCI.VXD` version
4.00.950, the exact PCI device/function/revision/subsystem IDs and the
PCIe bridge enumeration path remain separate verification gates.

The RTL8168/RTL8111 family shares PCI vendor/device ID `10EC:8168`;
that ID is **not** enough to distinguish RTL8111E from other revisions.
`drivers/network/rtl8111e/` now contains a portable data-only core
which checks Ethernet-class PCI identity (`02:00`), classifies externally
supplied `TxConfig` (register offset `0x40`) signature snapshots into
E1/E2/EVL candidates, and validates externally supplied MAC-address bytes.
Revision masks follow the Linux `r8169` chip table; unsupported signatures
remain *family candidates*, not asserted RTL8111E matches. No source code
in this module enumerates PCI, maps BARs, reads MMIO, or touches hardware.

Run `./build smoke-rtl8111e` for pinned LLVM i386 COFF compilation
and native-host regression tests (provided pinned toolchain is built).
An independent host Clang check also compiled the same source into
i386 COFF and passed the test vectors. This is **not yet an installable
Windows 95 driver**: there is no NDIS 3.1 miniport registration, LE
binary, hardware-connected DMA, IRQ handling, PHY management, transmit/receive path,
or binding INF. Nothing has been loaded on the Pavilion.

Keep the existing `./build ndis31-ddk-check` historical NT 3.51 DDK
gate. A loadable driver also requires read-only PCI inventory and a
verified Windows 95 RTM bus-enumeration path. Do not substitute NDIS 2
DOS, NDIS 5, an NT `.SYS`, or later Win9x headers without an explicit
compatibility analysis.

### RTL8111E DMA descriptor and ring core (2026-10-23)

The project now has an actual descriptor/ring **software implementation** in
`drivers/network/rtl8111e/dma_ring.h` and `dma_ring.c`, plus host-side
simulation vectors `tests/rtl8111e-dma-ring.c`. Unlike the earlier
identification and lifecycle-only code, this module writes and consumes
RTL8168-family **16-byte** TX/RX descriptors in little-endian byte layout:
`opts1`, `opts2`, address low and address high. It controls OWN
(bit 31), ring-end/EOR (bit 30), first-fragment/FS (bit 29), and
last-fragment/LS (bit 28). It checks the RX error summary, rejects
fragmented frames, and subtracts the four FCS bytes from valid RX
lengths. Offloads, jumbo frames and chained fragments are intentionally
not enabled.

**Boundaries and safeguards:**
- Only caller-supplied **already-mapped bus addresses**, never CPU virtual
  pointers converted into physical addresses. The first implementation
  conservatively requires a 32-bit address range, aligned 256-byte ring
  base, and eight 16-byte descriptors per ring. These are project policy
  limits, **not** claims about the RTL8111E's maximum DMA capabilities.
- Bounded 60–2048-byte TX frames and 64–2048-byte RX buffers; rejects
  address overflow, zero/oversized lengths, queues at capacity, and
  rebinding live rings. Buffer addresses cannot cross the 4 GiB ceiling.
- A six-function DMA **platform hook contract** supplies data synchronization,
  descriptor preparation **before OWN**, publication **after OWN**,
  completion acquisition, read barriers after cleared OWN, and RX data
  synchronization. It MUST be implemented by authenticated historical
  NDIS/OS services. A compiler fence or no-op hook is insufficient.
- Hardware completion does **not** automatically retire NDIS ownership.
  `whp_dma_complete` returns the descriptor result and leaves the
  buffer **CPU-held**; only `whp_dma_ack` after NDIS TX completion or
  RX payload copying/drop can retire its lifecycle ticket. Reset/halt
  remains blocked until both DMA rings and the lifecycle core are drained.
- Rejects suspicious device-modified buffer addresses or EOR bits and
  never releases DMA memory while ownership is unsettled. Synchronization
  calls must be serialized by the platform adapter; the ring is not
  inherently thread-safe.
- `whp_dma_unbind` only permits a quiesced HALTING/HALTED/RESETTING
  state and zero outstanding tickets. The verified platform must first
  disable DMA and interrupts, unmap buffers and release actual physical
  allocations; this core **does not** perform those privileged actions.

Run `./build smoke-rtl8111e` to compile `dma_ring.c` as an i386
Windows COFF object and exercise its simulator against the real portable
miniport lifecycle core. The source was also independently compiled
to i386 COFF and the DMA vectors passed locally under AddressSanitizer
and UBSan using a lightweight lifecycle mock. Those local tests are NOT
authentic NDIS driver tests.

The RTL8111E NDIS 3.1 adapter still returns failure on hardware
initialization, because the **NT 3.51 DDK DMA allocation and cache
synchronization APIs, actual PCI BAR/bus-master ownership, DMA masks,
interrupt masking and physical driver image have not been verified**.
Do not set the NIC's TX/RX ring registers or bus-master command bit
based solely on these simulated descriptors.

Hardware layout/reference witness (Linux r8169):
https://kernel.googlesource.com/pub/scm/linux/kernel/git/torvalds/linux/+/906dde0f355bd97c080c215811ae7db1137c4af8/drivers/net/ethernet/realtek/r8169.c

NDIS shared memory context (modern documentation, NOT NT 3.51 ABI
verification):
https://learn.microsoft.com/en-us/windows-hardware/drivers/network/using-shared-memory-in-network-drivers

### RTL8111E miniport lifecycle core (2026-10-23)

`miniport_core.h` and `miniport_core.c` provide **hardware-independent**
miniport lifecycle policy, not a loadable NDIS driver or a replacement DDK.
The core implements HALTED → INITIALIZING → RUNNING and explicit
RESETTING/HALTING drain paths. Initialization rejects unknown family
signatures and unusable MAC addresses. TX and RX have independently bounded
8-ticket bookkeeping pools with monotonically increasing serials; stale,
duplicate, out-of-range and wrong-direction completions are rejected.
Exhausting the serial space fails closed rather than reusing a ticket.

After reset/halt starts, new TX/RX work is refused. The drain operation
must complete/fail its **actual NDIS requests and receive indications**
before calling the corresponding ticket-retirement functions, and both
pools must be empty before reset/halt finishes. Failure to drain returns
`WHP_MP_BUSY` without discarding ownership. A failed reset returns to
HALTED; it does not silently resume network traffic.

**Concurrency boundary:** this core has no lock and is not thread-safe.
The eventual NT 3.51/Win95 NDIS miniport adapter MUST serialize all
calls with verified historical synchronization, and MUST own hardware
memory barriers, interrupt masking, DMA synchronization, NDIS completion
semantics and timeouts. The internal ticket cap is a software testing
limit, **not** a claimed RTL8111E DMA ring size. Do not invoke the core
from concurrently executing interrupt/send/reset code without that lock.
The PCIe/PnP enumeration and historical NDIS headers remain unverified.

Run `./build smoke-rtl8111e` to compile the identification *and* miniport
policy sources as i386 COFF, run host-only identity tests, and run the
miniport lifecycle regression. `./build.sh menuconfig` also offers
`rtl8111e`, and the `all` profile now includes its smoke check.
No DMA, IRQ, PHY, real packet path, `.VXD`, INF binding or RTL8111E
hardware access has been enabled.

Evidence:
- Microsoft Windows 95 DDK network model:
  https://techshelps.github.io/MSDN/WIN95DDK/HTML/S2D14.HTM
- Linux `r8169_main.c` RTL8168E/8111E revision mask and TxConfig:
  https://linux.googlesource.com/linux/kernel/git/torvalds/linux/+/6c790212c588fddeb0d852f2790840753bb604b1/drivers/net/ethernet/realtek/r8169_main.c
