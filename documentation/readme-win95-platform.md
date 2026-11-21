# WHP-Drivers — Windows 95 hotfix, DDK, network ABI, and setup evidence

This document preserves detailed research moved from the root README. Commands and relative source paths in code blocks are relative to the **repository root**. See [the project overview](../README.md) for the shortest supported entry points.

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
### LLVM/COFF checks for NT 3.51 miniport development (2026-10-23)

The RTL8111E portable cores use the pinned LLVM/Clang fork with the
explicit `--target=i386-pc-windows-msvc` ABI. An i386 COFF machine
header alone cannot establish NDIS `__stdcall` calling conventions,
wrapper symbol decoration, historical import libraries, the PE driver
image flags or a Windows 95 RTM load.

The synthetic NDIS adapter header previously defined `NTAPI` as empty.
That tested callback shapes but masked cdecl/stdcall mismatches when
compiling the 32-bit Windows object. The fixture now declares synthetic
i386 callbacks and wrapper functions as `__stdcall`, with explicit
decorated symbol assertions. It is still **not** the NT 3.51 DDK.

Two explicit pinned-compiler audit commands are now provided:

```sh
./build llvm-ndis31-abi-check
./build llvm-ndis31-core-audit
./build test-ndis31-llvm
```

The ABI check compiles a synthetic C fixture with pinned LLVM and verifies
the actual COFF names for cdecl, stdcall, direct NDIS-style calls, and
`dllimport` calls. These are deliberately **contrasting** examples:
which import form the genuine historical DDK uses is still unverified.

The core audit compiles `identity.c`, `miniport_core.c`, and
`dma_ring.c`, then parses their real i386 COFF symbols, checks
inter-object references, and rejects implicit `.drectve`
default-library directives, unexpected CRT/TLS sections and unverified
runtime helpers such as `memcpy`, `__chkstk` and security-cookie calls.
It is read-only on the produced object files and makes no NDIS imports.
Failures expose exact unexpected symbol names instead of silently
pulling a newer CRT or disabling protections.

The verifier is `tools/check_ndis31_llvm.py`. Six regression cases cover
actual Clang decoration, wrong-machine objects, unexpected default libs,
core symbol resolution, helper imports, and corrupt objects. The
`rtl8111e` and `all` fast/full test profiles exercise these synthetic
cases, without a historic kit or an expensive LLVM bootstrap.

**LLVM fork status:** No incorrect compiler lowering has been established
yet; do not patch LLVM without a failing pinned-fork test. WHP-Drivers
continues to pin `9137c36d8aeb475cf89d7f330e4cf39b49241ab8`.
Water's pin is newer but does not automatically mean its changes are
required here. Authentic NT 3.51 header ABI, real `ndis.lib`, PE loader
and DMA cache hooks remain independently unverified. These commands do
not produce or install a Windows 95 miniport.

Reference (x86 symbol decoration):
https://learn.microsoft.com/en-us/cpp/build/reference/decorated-names

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
