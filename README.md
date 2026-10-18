# WHP-Drivers

WHP hardware-driver research and development.

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
No Water runtime or library is added to the VxD link by default. Historical
VMM/VxD headers and LE service linking remain separate, unverified work.
Use the recorded gitlink revision, not the moving Water `master` branch.

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
