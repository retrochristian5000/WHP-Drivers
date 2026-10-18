# WHP-Drivers

WHP hardware-driver research and development.

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

## Driver format boundary

Windows 95 RTM VxDs require historical VMM/VxD interfaces and a valid LE
(Linear Executable) driver. Compiling a COFF object with Clang does not
produce a loadable `.VXD`; LLD's PE/COFF linker must not be assumed to
emit LE. Before driver linking, validate an LE-capable linker and appropriate
historical DDK definitions independently. Native NT `.SYS`, WDM, and
Win9x VxD driver contracts are separate.
