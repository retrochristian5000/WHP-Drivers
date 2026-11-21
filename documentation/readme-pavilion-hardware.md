# WHP-Drivers — Pavilion USB, AMD FCH SATA, and PCIe topology

This document preserves detailed research moved from the root README. Commands and relative source paths in code blocks are relative to the **repository root**. See [the project overview](../README.md) for the shortest supported entry points.

### Pavilion p6-2310 USB host-controller inventory (2026-11-18)

The **AMD A55/Hudson-D2** Pavilion model advertises USB 2.0 ports.
The likely built-in controller arrangement is **OHCI + EHCI**:
these are separate PCI USB controller functions even when they
service different device speeds through the same physical port.
Do not count host-controller functions as individual USB connectors.

PCI class `0Ch`, subclass `03h` and programming-interface (PI)
byte identify a USB controller: `00h` UHCI, `10h` OHCI,
`20h` EHCI, `30h` xHCI, `80h` unspecified. PI `FEh` denotes
a USB *device* rather than a host controller and is not inventoried.
Unknown controller PIs are reported as unknown, not coerced to EHCI.

Expected AMD FCH **family** PCI IDs (not Pavilion-observed IDs):

| Vendor:device | Controller interface | Speed interpretation |
| --- | --- | --- |
| `1022:7807` | OHCI | low/full, 1.5/12 Mb/s |
| `1022:7808` | EHCI | high, 480 Mb/s; companion routing needed for slower devices |
| `1022:7809` | OHCI | low/full, 1.5/12 Mb/s |
| `1022:7812`, `1022:7814` | xHCI family IDs | **not** evidence that this A55 board has USB 3.x |

These numbers are *interface capability classes*, not actual USB
device negotiation rates, throughput measurements, physical port
topology, device identities, or OS driver support. In particular,
`1022:780B` is catalogued as AMD FCH **SMBus** and is never
classified as USB from the numeric ID alone. If observed PCI class
and family ID expectations disagree, preserve both in the report.

`drivers/usb/pci_hci.[ch]` contains a 64-byte, pure PCI-header
decoder. The DOS `WHPPROBE.EXE` front end now reports all matching
USB host-controller functions (including non-AMD UHCI and xHCI)
without enabling ports, reading memory-mapped registers, moving
OHCI/EHCI ownership, accessing devices, changing IRQ or DMA state,
or invoking the Windows USB driver stack. Test with:

```sh
./build test-usb-pci-hci
WHP_DRIVER_PROFILE=dos-probe ./build.sh test-fast
```

**Historical Windows split:** Windows 95 RTM lacks the later USB
Supplement; Windows 95 OSR2.1/2.5 can use their release-specific
USB 1.x supplement. This does **not** turn EHCI into a native
Windows 95 RTM USB 2.0 driver. Windows 98/98 SE also require
appropriate EHCI drivers for high-speed transfers. Genuine DOS
6.22 and 7.x may expose BIOS legacy keyboard/storage emulation
without owning USB host-controller hardware. Firmware-mode handoff,
host-controller driver, root hub, device enumeration and class
driver remain distinct milestones. A DOS PCI BIOS listing proves
none of these OS driver paths.

References:
- PCI IDs: https://kernel.googlesource.com/pub/scm/utils/pciutils/pciutils.git/+/5371aab4803635ddb5d1f53eeade17a598352593/pci.ids
- USB interface/companion routing: https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/usb-faq--introductory-level
- Microsoft Windows 95 OSR2.x USB Supplement archival KB Q181661:
  https://jeffpar.github.io/kbarchive/kb/181/Q181661/

### Pavilion p6-2310 AMD A55 SATA/RAID identification (2026-11-18)

The HP Pavilion **p6-2310 (H3Y76AA)** uses the AMD A55/Hudson-D2
FCH. A BIOS/UEFI hard-drive selection that offers **RAID** is an
**observed setup option**, not proof that RAID mode is selected, that
a RAID volume exists, or that a standalone RAID controller is fitted.
The actual storage function PCI IDs, active subclass/PI and
attached drives still need a read-only machine capture.

A pure PCI-configuration decoder lives in
`drivers/storage/fch_sata/identity.[ch]`; it recognizes mass-storage
PCI class `01h` with IDE subclass `01h`, RAID subclass `04h`,
or SATA subclass `06h` (AHCI programming interface `01h`).
It reports actual class/PI separately from family ID expectations.
AMD vendor `1022h` candidate family IDs:

| Device ID | Family interpretation (not Pavilion-confirmed) |
| --- | --- |
| `7800h` | FCH/Hudson SATA IDE mode |
| `7801h` | FCH/Hudson SATA AHCI mode |
| `7802h` | FCH/Hudson SATA RAID mode |
| `7803h` | FCH/Hudson SATA RAID5-related variant |
| `7804h` | FCH/Hudson alternate AHCI mode |
| `7805h` | FCH/Hudson alternate RAID mode |

`780Ch` is separately listed as an FCH **IDE controller**:
do not silently merge it into the six SATA mode IDs. Candidate
IDs may be seen on other AMD FCH configurations and are not a
verified inventory of this particular board.

The existing `WHPPROBE.EXE` now prints any detected IDE/SATA/RAID
storage-class endpoint, including vendor/device, class/subclass/PI,
revision, subsystem pair, command flags and IRQ line/pin. A family-ID
versus PCI-class disagreement is a diagnostic clue, not permission
to select a different driver. It **does not** read a disk sector,
write BARs, access MMIO, inspect a RAID array or access Option ROM metadata.
The separately gated DOS memory preflight can now **read PCI BAR address
fields only**, without claiming that their assigned size is known.
No Windows 95 storage driver, AHCI port initialization, DOS block
driver or RAID volume assembly is implemented by this step.

Do **not** toggle the firmware SATA mode on the existing Windows 95
installation merely to test the candidate IDs; doing so could
make the current boot volume inaccessible. First record the mode
currently selected and capture the read-only PCI listing. In
Windows 95, real-mode DOS disk access, `IOS.VXD`,
`ESDI_506.PDR`, protected-mode controller binding and native RAID
are different paths. Real-mode BIOS INT 13h access is not proof
that Win95 RTM can drive this controller in protected mode.
MS-DOS 6.22 versus 7.10 FAT support is another independent
boundary and does not identify an active RAID configuration.

Host-only, synthetic identity tests (no LLVM, OpenWatcom or disks):

```sh
./build test-fch-sata-identity
WHP_DRIVER_PROFILE=dos-probe ./build.sh test-fast
```

Research references:
- [PCI ID ledger for AMD 1022:7800–7805 and 780C](https://kernel.googlesource.com/pub/scm/utils/pciutils/pciutils/+/be40c4738eef9d1283207e62f539c52a5315c10c/pci.ids)
- [Hudson SATA firmware source, IDE/AHCI/RAID families](https://chromium.googlesource.com/chromiumos/third_party/coreboot/+/796af17f18554380a49d69d7768ac18ee039d711/src/southbridge/amd/agesa/hudson/Kconfig)
- [HP information about SATA mode and boot implications](https://support.hp.com/my-en/document/ish_9631042-9631101-16)

### AMD Family 12h PCIe root complex and Hudson FCH topology (2026-11-18)

AMD's own primary source resolves the main architecture boundary:
**BIOS and Kernel Developer's Guide for AMD Family 12h Processors**,
document **41131**, revision **3.02**, October 6, 2011, section 2.11
("Root complex"), especially **Figure 11** (printed p. 124).
The Llano APU includes a PCIe Gen 2 root complex. Its advertised bus 0
PCI device positions are:

| Bus:device.function | AMD Family 12h function |
| --- | --- |
| `00:02.0` | GFX0 root port (can combine with GFX1) |
| `00:03.0` | GFX1 root port |
| `00:04.0` | GPP0 root port |
| `00:05.0` | GPP1 root port |
| `00:06.0` | GPP2 root port |
| `00:07.0` | GPP3 root port |
| `00:08.0` | UMI bridge to FCH; **hidden by default** |

AMD's BKDG says the GFX links can be two x8 links or one x16
and the GPP group includes four general-purpose ports plus the
separate **x4 UMI interconnect** to the FCH. Thus UMI, GPU,
network endpoint, PCIe root port, and FCH bridge must not be blurred
into one "PCIe controller". The described BDF layout is a *processor
family topology*, not a promise that every bridge appears on the
Holly2 motherboard.

The related **Hudson-family FCH** has its *own* PCIe port family,
with PCI IDs `1022:43A0`–`1022:43A3`. They correspond to
FCH GPP0–GPP3 at `00:15.0`–`00:15.3` on documented
Hudson/Bolton-family configurations. The 2014 AMD Bolton FCH
BIOS Developer's Guide confirms these functions, but because Bolton
is later than Hudson-D2, it is **corroborating family-level evidence,
not an A55-specific register programming mandate**.

`drivers/bus/pci95/amd12_topology.[ch]` identifies only observed,
matching **AMD `1022h` type-1 PCI-to-PCI bridge headers** at
these family locations. The APU family-ID range used for matching
is `1706h`–`170Dh`; the FCH functions require their matching
`43A0h`–`43A3h` IDs. IDs and locations are diagnostic
filters, *not* proof of the actual physical link routing.

Where a valid standard PCIe capability is already available in the
bounded 64–256 byte PCI snapshot, the decoder reports the
**Link Status** register's negotiated speed code, negotiated width
and Data Link Layer Link Active status bit. For the Family 12h Gen 2
hardware, speed code `1` corresponds to 2.5 GT/s and `2` to
5 GT/s. A zero or absent result is **not** evidence that a physical
slot or controller is missing; link-status capability/firmware
availability and negotiated link state are separate observations.
A reported link-active bit also does not verify a working driver.

The existing `WHPPROBE.EXE` bridge report now labels these
documented port-family candidates without writing AMD indexed
configuration registers (including `D[8:2]F0xE0/E4`), altering
link training or changing bus numbers. The earlier hardcoded
"bus 3" diagnostic has been removed in favor of each observed
bridge's declared bus range. No Windows 95 PCI.VXD, PCI IRQ steering,
root-port initialization or PCIe resource reassignment is implemented.

```sh
./build test-amd12-pcie-topology
WHP_DRIVER_PROFILE=dos-probe ./build.sh test-fast
```

Primary/secondary evidence:
- AMD Family 12h BKDG, section 2.11, rev. 3.02:
  https://www.amd.com/content/dam/amd/en/documents/archived-tech-docs/programmer-references/12h_bkdg_pub.pdf
- AMD Bolton FCH BIOS Developer's Guide, section 2.1 (later-family corroboration):
  https://www.amd.com/content/dam/amd/en/documents/archived-tech-docs/programmer-references/51205_Bolton_FCH_BIOS_Dev_Guide.pdf
- PCI IDs for AMD Family 12h root ports and Hudson FCH bridges:
  https://kernel.googlesource.com/pub/scm/utils/pciutils/pciutils/+/e2d9340ba416ef0856fb8198e16ffd255225952c/pci.ids

### AMD A55 / Windows 95 PCI-to-PCI bridge preflight (2026-11-18)

The HP Pavilion p6-2310 H3Y76AA specification identifies the **AMD A55
FCH** and PCI Express expansion slots, but this does **not** identify
each PCI root port or PCI-to-PCI bridge in the actual machine. Root
complex/host bridges, type-1 downstream bridges, and endpoint devices
have different configuration layouts and must not be conflated.

The new independent `drivers/bus/pci95/pci_bridge.[ch]` module decodes
**caller-supplied PCI configuration snapshots only**. It validates a
64-byte type-1 PCI-to-PCI bridge header (class 06:04), records primary,
secondary and subordinate bus numbers, read-only PCI command bits,
BAR0/1 raw values, legacy interrupt pin/line, bridge control, and
assigned I/O, non-prefetchable and prefetchable memory windows (including
32-bit I/O and 64-bit prefetchable ranges). It also checks advertised
standard PCI capabilities using a bounded and cycle-resistant parser.
The results distinguish declared bus-range membership from real endpoint
enumeration. A reported resource window does **not** prove that decoding
is enabled or that memory is accessible; the command register must be
examined separately. No PCI writes, BAR-sizing probes, IRQ routing,
physical memory mapping or firmware calls are made.

```sh
./build test-pci95-bridge
WHP_DRIVER_PROFILE=rtl8111e ./build.sh test-fast
./build smoke-rtl8111e    # also checks PCI95 as i386 COFF, needs pinned LLVM
```

The first command uses a normal native C compiler; there is no LLVM
bootstrap, historical DDK, Windows 95 guest, or real device access.
Synthetic tests cover BIOS-unassigned child buses, invalid range
membership, inaccessible windows, 64-bit prefetchable windows and
malformed or truncated PCI capability chains. These tests are a
**research and regression baseline, not an installable PCI controller
driver**. The test's AMD vendor and synthetic device ID are fabricated
input, not an HP Pavilion hardware inventory.

**Windows 95 version boundary:** retail Windows 95 4.00.950 relies on
the firmware to configure devices downstream of PCI-to-PCI bridges.
Retail and OSR1 also lack OSR2 PCI IRQ steering. A bridge whose child bus
is missing from Device Manager therefore requires a real read-only
PCI topology comparison before we can attribute the cause to either
firmware initialization, PCI.VXD enumeration or the RTL8111E miniport.
A loadable bus enumerator/VxD would additionally require authenticated
Win95 RTM Configuration Manager and PCI BIOS interfaces, resource
ownership/interrupt routing, correct LE linking, and disposable-guest
testing. Do not replace system PCI.VXD, update the chipset registers,
enable bridge decoding or bind the RTL8111E on the strength of this
decoder.

References:
- HP p6-2310 specification (AMD A55 FCH): https://images-na.ssl-images-amazon.com/images/G/15/electronics/detail-page/B009O6TU8Y.pdf
- Historical Windows 95 PCI-to-PCI bridge behavior (Microsoft KB159038):
  https://www.betaarchive.com/wiki/index.php/Microsoft_KB_Archive/159038
- Historical PCI IRQ steering behavior (Microsoft KB182604):
  https://helparchive.huntertur.net/document/105286
- PCI type-1 header constants:
  https://kernel.googlesource.com/pub/scm/linux/kernel/git/hpa/linux-extable/+/refs/heads/master/include/linux/pci_regs.h
