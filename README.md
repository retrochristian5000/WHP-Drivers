# WHP-Drivers

Research and incremental driver development for legacy PCs and selected modern
host systems. The repository includes Windows 95-era ABI/driver experiments,
DOS diagnostics, and portable or macOS-specific hardware observation tools.
**Most targets remain research-stage, not installable or functional drivers.**

## Quick start

```sh
git clone --recurse-submodules https://github.com/retrochristian5000/WHP-Drivers.git
cd WHP-Drivers

./build.sh plan             # inspect the selected steps; no build
./build.sh menuconfig       # select a profile and save settings
./build.sh test-fast        # profile-specific host regressions
./build.sh build            # run the chosen smoke/build profile
./build.sh test             # fuller checks; may require pinned toolchains
```

The POSIX-shell `./build.sh` frontend delegates to the existing `./build`
backend; its legacy targets remain available. Configuration is validated
data in ignored `.whpconfig`, overridable with `WHP_DRIVER_PROFILE`,
`WHP_DRIVER_BOOTSTRAP`, `WHP_DRIVER_VXD_BACKEND`, and `WHP_LLVM_JOBS`.
Toolchain bootstrap defaults to **off**; it is always explicit. No ordinary
build command installs drivers, updates submodule revisions, or writes to
physical devices.

Useful additional commands:

```sh
./build.sh config           # show effective settings
./build.sh incremental      # use existing toolchains; no bootstrap
./build.sh test-menuconfig  # isolated build-menu regression tests
./build.sh clean-tests      # guarded removal of generated test files only
./build.sh bootstrap        # explicitly build the pinned LLVM fork
./build.sh bootstrap-watcom # explicitly bootstrap pinned OpenWatcom
```

## Work areas and technical references

Keep detailed evidence, protocol analysis, command tables, and hardware notes
in the linked technical documentation rather than expanding this overview.

| Area | Current boundary | Technical reference |
| --- | --- | --- |
| Build system, Water, OpenWatcom, LLVM, LE/VxD | Pinned toolchains; Windows 95 loadable VxD linking is not yet established | [Build and toolchains](documentation/readme-build-toolchains.md) |
| Windows 95 ABI, hotfixes, installation, NDIS 3.1 | Header/ABI/INF research with provenance gates | [Windows 95 platform](documentation/readme-win95-platform.md) |
| AMD HDA, Win95 audio, graphics, Radeon HD 6410D | Read-only observations, test scaffolds; no sound/display driver | [Audio and display](documentation/readme-audio-display.md) |
| Realtek RTL8111E | PCI identification and NDIS miniport scaffolds; no live DMA/network path | [RTL8111E research](documentation/readme-rtl8111e.md) |
| Pavilion p6-2310 USB controllers, SATA, AMD PCIe | Hardware-class inventory, not proof of an attached USB device or active RAID | [Pavilion hardware](documentation/readme-pavilion-hardware.md) |
| DOS `WHPPROBE.EXE` | Opt-in OpenWatcom MZ build and firmware/PCI diagnostics; not a USB device driver | [DOS probe and packaging](documentation/readme-dos-probe.md) |
| Brother MFC-290C printer/scanner/fax | USB identity, CUPS/IOKit observation, fax policy and **offline** USB command classification only | [MFC-290C evidence and protocol ledger](documentation/brother-mfc290c.md) |

### Targeted tests and probes

```sh
./build test-usb-pci-hci
./build test-hda-probe
./build test-brother-mfc290c

WHP_DRIVER_PROFILE=dos-probe ./build.sh plan
WHP_DRIVER_PROFILE=dos-probe ./build.sh build # opt-in 16-bit DOS toolchain

WHP_DRIVER_PROFILE=brother-mfc290c ./build.sh test-fast
./build.sh probe-brother-mfc290c-macos  # native macOS; read-only
```

The MFC-290C's documented USB identity is `04f9:01fd`; an observed macOS
`locationID=0x00120000` is a connection-topology hint, **not** a fixed
device identity. Standalone fax reception does not establish PC-FAX receiving,
and the actual Brother print/scan/fax USB commands remain unverified.
See the [Brother evidence ledger](documentation/brother-mfc290c.md) before
drawing conclusions from a shared model family or standard USB request.

### Shared DOS and macOS probe core

The DOS `WHPPROBE.EXE` and the macOS MFC-290C observer now
use one **portable evidence-source core** at
`tools/probe/probe_scope.{h,c}`. It reports the backend and
what that backend can actually see; it contains no operating-system
headers or hardware I/O. The executable formats, compilers,
runtime entry points and hardware access remain separate:

- **DOS:** 16-bit OpenWatcom MZ `WHPPROBE.EXE`, BIOS/PCI
  scanning, COM/LPT/memory discovery and DOS menu, all under
  `tools/dos/`. PCI USB-controller detection does **not**
  enumerate connected USB printers.
- **macOS:** native user-space Mach-O
  `mfc290c-probe`, IOKit/CUPS metadata, and a process-ISA
  check under `tools/macos/`. Registry observations do **not**
  claim raw descriptor or functional driver support.

Both tools keep their existing arguments. The shared source line
uses `usbDevices=not-enumerated` for DOS, `registry-only`
for the Mac's USB observer, and `not-queried` for the Mac's
`--architecture-only` mode. It never reports a peripheral
from a DOS PCI-controller observation.

```sh
./build test-probe-scope        # pure C89, no hardware access
./build test-probe-menu         # DOS menu + shared scope test
./build test-brother-mfc290c    # USB model tests + shared scope test
```

See the [DOS probe](documentation/readme-dos-probe.md)
and [MFC-290C evidence](documentation/brother-mfc290c.md)
for platform-specific work.

## Validation and development rules

- **Evidence first:** preserve model, OS/ABI, source provenance, negative
  results, and the difference between synthetic tests and physical hardware.
- **Fail closed:** a successful compiler smoke test or PCI/USB identity match
  does not establish a working driver, interface protocol, firmware behavior,
  or safe installation.
- **Avoid unintended I/O:** diagnostics and tests must not silently reset
  devices, enable DMA, print, scan, dial fax numbers, or modify firmware.
- **Keep changes focused:** retain existing `main` and update the relevant
  technical reference when adding new research. Extend this README only when
  a new top-level entry point or project area warrants a link.

Repository automation lives in [GitHub Actions](.github/workflows/); some
checks require a particular host operating system or compiler. Confirm each
workflow's reported result independently—its presence does not imply a
successful physical-device test.
