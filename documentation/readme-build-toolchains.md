# WHP-Drivers — Build entry point, Water, OpenWatcom, LLVM, VxD/LE constraints

This document preserves detailed research moved from the root README. Commands and relative source paths in code blocks are relative to the **repository root**. See [the project overview](../README.md) for the shortest supported entry points.

## Configurable build entry point

Use `./build.sh` as the new top-level entry point; the historical executable
`./build` remains intact as a backend and preserves all existing commands.

```sh
./build.sh menuconfig         # interactive portable shell menu, save with 's'
./build.sh config             # inspect active options
./build.sh plan               # show exact configured backend steps (read-only)
./build.sh build              # run configured smoke profile (default: ABI)
./build.sh incremental        # build using existing toolchains; never bootstrap
./build.sh test-fast          # host regressions; does not build LLVM
./build.sh test               # full format, LE, and selected driver checks
./build.sh clean-tests        # remove generated test outputs, not toolchains
./build.sh clean-dos-probe      # remove only DOS PCI build outputs
./build.sh test-menuconfig    # isolated shell-menu regression checks
./build.sh bootstrap          # explicitly compile the pinned LLVM fork
./build.sh bootstrap-watcom   # bootstrap pinned OpenWatcom host + release tools
./build.sh bootstrap-all      # build both pinned toolchains
WHP_DRIVER_PROFILE=dos-probe ./build.sh plan      # DOS-only plan
WHP_DRIVER_PROFILE=dos-probe ./build.sh build     # opt-in DOS MZ executable
WHP_DRIVER_PROFILE=hda ./build.sh test-fast  # HDA PCI/GCAP host-only checks
WHP_DRIVER_PROFILE=brother-mfc290c ./build.sh test-fast  # USB identity tests
```

The menu selects one workflow (`abi`, `vxd`, `radeon6410d`, `rtl8111e`, `hda`, `brother-mfc290c`, `dos-probe`, or `all`),
the VxD backend (`llvm` or optional `openwatcom`), and **independent**
toolchain bootstrap (`off`, `llvm`, `openwatcom`, or `both`), plus LLVM
parallel jobs (1–16, default 2). Earlier `WHP_DRIVER_BOOTSTRAP=on` profiles
are accepted and interpreted as `llvm`; `off` remains the default.
The `dos-probe` profile uses pinned OpenWatcom's 16-bit DOS compiler and is
not implicitly added to the existing `all` profile. Ordinary
`test-fast` DOS checks use only host C/Python, not a DOS emulator or LLVM.
The Radeon identity profile remains LLVM-based; the backend choice affects
**only the VxD smoke probe**. OpenWatcom is never a silent fallback, and
the VxD backend must pass its own LE verification. OpenWatcom bootstrap
is independent of whichever backend runs the VxD probe.

**Workflow consistency (2026-10-23):** `plan` uses the same step dispatcher
as `build`, rather than independently guessing compiler commands.
`incremental` disables bootstrap **for that invocation only** without
changing `.whpconfig` or updating any submodules; an absent compiler
will still produce an explicit failure. `test-fast` exercises the local
format checks and, for selected profiles, Radeon INF and synthetic NDIS
3.1 declaration/library/adapter regressions. It does not require a
historical NT 3.51 DDK or bootstrap a toolchain, although the NDIS
adapter tests may use a host compiler. Full `test` preserves the
LE and selected smoke probes and adds the NIC regressions for
`rtl8111e`/`all`. `clean-tests` refuses symlinked test-output paths and
custom LLVM build directories; it only deletes the default
`.build/llvm-native/whp-tests` directory. Neither command installs
drivers, invents a validated NDIS miniport binary, or enables hardware DMA.

Settings are stored in ignored `.whpconfig` as validated data (not
shell-sourced); environment overrides are `WHP_DRIVER_PROFILE`,
`WHP_DRIVER_VXD_BACKEND`, `WHP_DRIVER_BOOTSTRAP`, and `WHP_LLVM_JOBS`.
The front end performs **no automatic Git updates**, submodule pin changes,
driver installation, or GPU access. On an existing checkout, `./build`
still works exactly as before. A fresh checkout needs the pinned LLVM
submodule and a built LLVM toolchain before ordinary smoke commands work
unless LLVM bootstrap is explicitly selected. This menu is a portable
text interface, not a curses dependency.


### Build command organization (2026-11-21)

Use `./build.sh --help` for commands grouped by discovery,
building/testing, explicit toolchain bootstrapping, cleanup and backend
pass-through. `./build.sh profiles` enumerates supported profiles and
explains which workflows are excluded from `all`. In `menuconfig`,
**p** selects a profile by its exact name and **b** selects a bootstrap
mode directly. The original numeric 1-4 cycle/toggle controls remain
available. `dos-pci` remains valid for saved legacy configurations,
while new DOS work should select `dos-probe`.

The frontend's DOS host checks and NDIS regressions now have named
dispatch helpers, without changing their commands or execution order.
This organization change does **not** change compiler flags, hardware
access, CI cache keys, saved configuration format, cleanup boundaries
or `incremental` behavior. Invalid direct selections leave the
previous menu values unchanged.

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

### OpenWatcom fork reliability audit (2026-11-18)

The pinned secondary toolchain remains explicitly selected and never
silently replaces the LLVM path. Two bootstrap failures were corrected
in the OpenWatcom fork before updating the `toolchains/openwatcom`
gitlink:

1. POSIX `ci/buildx.sh` once discarded the exit status of
   `builder -i test` after printing `result.log`, and continued
   across failed documentation sub-bootstraps. It now preserves the
   first stage failure, rejects unknown stage names, and makes
   bootstrap output directory creation repeatable.
2. `cmnvars.sh` could report an old or zero compiler version after
   failed Clang/GCC/Watcom host version probing. Failed preprocessors
   and invalid versions now fail closed instead of giving later driver
   builds ambiguous compiler provenance.

The fork carries isolated POSIX mock tests for both failure classes,
and its own lightweight GitHub Actions job passed. WHP-Drivers now
also fetches **only four shell source files** at the pinned OpenWatcom
commit for a separate host-only regression run. This adds no LLVM,
OpenWatcom binaries, source checkout, or release-build expense to
ordinary CI.

The following remain separate and **unverified**: actual macOS A18
Pro OpenWatcom bootstrap, 16-bit DOS MZ linking, Win16 audio or
display `.DRV` objects, Win95 VxD **LE** output, and real Windows
95 RTM driver loading. The historical OpenWatcom
`FORMAT WINDOWS VXD DYNAMIC` issue cannot be declared fixed from
shell-script tests alone; the independent `verify_vxd.py` LE
structural gate is still mandatory.

Source-level checks are cheap; full compiler bootstrap is an
explicit second stage:

```sh
git submodule update --init toolchains/openwatcom
./build watcom-check          # requires existing built host tools
./build watcom-smoke-vxd      # only if a built Watcom linker exists
```

### DOS bootstrap compiler consistency (2026-11-21)

The pinned fork now forwards the selected `OWCLANG` to the POSIX
`wmake` bootstrap via `make ... CC=...` and to the `builder` preboot
via `wmake ... CL=...`. Previously, both stages could use the host's
default `cc` even when the initial compiler probe selected Clang.
The default `OWDEFPATH` no longer appends an empty `PATH` element,
which could cause an implicit search of the current directory.

Isolated regression tests check compiler selection, missing-compiler
failure, and the unchanged inherited PATH. These are **mocked host
bootstrap checks**, not proof of a completed OpenWatcom release or
a working 16-bit DOS executable. The DOS source-build workflow must
still compile, link, and independently verify `WHPPROBE.EXE` before
its archives are accepted.

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
