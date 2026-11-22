# Brother MFC-290C — Firmware and service-interface evidence

**Status:** research and read-only observations only. **No firmware image,
updater implementation, flash write, EEPROM write, or USB service request
has been recovered or authorized.** This page extends the existing
[MFC-290C technical ledger](brother-mfc290c.md), not the root README.

## Confirmed firmware-update support

Brother's own MFC-290C download listing contains a **Firmware Update Tool**,
described as obtaining firmware from Brother's internet server and sending
it to the machine. The US listing shows a firmware **release marked G**
dated **2011-09-08** (the listed tool download size is **20.11 MB**).
These are historical catalogue metadata: **G is not an observation of the
firmware currently installed on the user's MFC-290C**, and the tool's file
size does **not** measure the flash image.

Official model page: https://support.brother.com/g/b/producttop.aspx?c=us&lang=en&prod=mfc290c_all

Official release listing (OS-specific):
https://support.brother.com/g/b/downloadlist.aspx?c=us&lang=en&os=10013&prod=mfc290c_all

Brother's 2008 service manual **SM-FAX098**, covering MFC-250C/290C/490CW/
790CW/990CW and DCP relatives, describes three **distinct mutable domains**:

| Firmware domain | Documented purpose | Driver research boundary |
| --- | --- | --- |
| **Flash ROM on main PCB** | Holds machine firmware; Appendix 2 addresses installation/replacement from a host PC | This implies a firmware delivery and recovery mechanism, **not a publicly documented standard USB command format** |
| **EEPROM customization** | Records region/model settings, paper/printhead and scanner calibration data; Appendix 3 allows common firmware to be customized for different models | Shared base firmware **does not** make firmware images or service commands safely interchangeable |
| **Firmware switches (WSW)** | Configuration preferences for destination or operating conditions (Appendix 4) | Software-controlled functions may vary by settings, but **WSW values are not USB SETUP opcodes** |

The manual's Chapter 7 refers to a **Maintenance USB Printer Driver**, and
the service procedures distinguish maintenance-mode settings from normal
machine operation. This is a *different service software context*, not
proof of any alternate USB product ID, endpoint, bootloader protocol,
or a working macOS maintenance driver. No authenticated service-mode USB
descriptor or flash packet has been recorded.

Service-manual contents and version-key evidence:
https://manualmachine.com/brother/dcp145c/18240960-service-manual/

## Distinguishing firmware from USB metadata

| Evidence | Where to find it | Status |
| --- | --- | --- |
| **Installed firmware** | Physical MFC-290C LCD | **Not observed yet** |
| **Published firmware release** | Brother's MFC-290C support download catalogue | **G**, 2011-09-08; not an installed version |
| **USB device release** | USB descriptor / macOS IOKit `bcdDevice` | Host-visible device revision; **not proved to encode the LCD firmware letter** |

These are **observations from different sources**, not firmware
variables maintained by our driver. Our current code cannot read
the firmware letter, translate `bcdDevice` into an installed
firmware version, or select a command set from either value.

Brother's read-only instruction for this model is to press **`*` and
`#` simultaneously**; the LCD shows a firmware-version letter in the
first or second position from the left. Record the observed letter exactly,
without treating it as a decimal/BCD value. This control-panel check does
**not require** entering maintenance mode or installing the updater.

Official model-specific FAQ (2014-02-20):
https://support.brother.com/g/b/faqend.aspx?c=nz&faqid=faq00100000_005&ftype3=2037&lang=en&pfs=1&prod=mfc290c_all

The portable driver identity `04f9:01fd` and previously observed macOS
`locationID=0x00120000` are **different** evidence types again:
identity and host topology, respectively. None of these values should
be substituted for a firmware version.

## How firmware may explain printing, scanning, and faxing

The main-board firmware controls the machine, but it is **unknown
whether different installed MFC-290C firmware revisions change USB
commands, endpoint behavior, scanning, print streams, panel events
or PC-FAX.** Firmware relevance must be tested, not assumed.

| Driver question | Sufficient evidence before adding a version-specific path |
| --- | --- |
| Different USB descriptors? | Comparable raw, read-only descriptors from identified units, with independently observed installed firmware letters |
| Different print/scan commands? | Authentic model-specific documentation or lawful equivalent captures showing repeatable changes while controlling host-driver version and interface context |
| Different panel, fax or media behavior? | Verified target-function mapping, then reproducible behavior across installed versions |
| Host incompatibility after an update? | Device/firmware plus host OS and driver revision recorded together; never infer from the published G listing |
| Service-mode or calibration differences? | Service-manual or updater documentation; **no experiments with EEPROM, flashing or resets** |

**Implementation baseline:** the present C11 code parses supplied
USB identity and captured SETUP bytes, and the macOS tool inspects
IORegistry/CUPS metadata. It does not query installed firmware or
send Brother commands. Avoid speculative firmware switches until
model-specific evidence demonstrates a real behavioral difference.
If no difference is established, one well-tested protocol path is
preferable to unverified version branches.

The firmware image layout, processor ISA, vendor opcodes, flash
memory map, cryptographic checks, and firmware-to-command
compatibility remain unverified. An available updater is not
proof of a command or a portable firmware image.

Preserve separate command families during analysis:

1. **Ordinary USB enumeration** — standard USB descriptor requests;
   `bcdDevice` remains a descriptor field.
2. **Normal print, scan, and PC-FAX transfers** — protocol not yet
   authenticated; PC-FAX sending must not be conflated with standalone
   fax reception.
3. **Maintenance/update mode** — potentially destructive service path;
   never classify maintenance USB traffic as the normal fax or scanner
   protocol without verified model-specific evidence.
4. **On-device fax/engine configuration** — EEPROM and WSW values are
   internal state, not proof of host USB class requests.

If legally available firmware or updater artifacts are inspected later,
begin with **static, offline inspection**: record model, region, download
date, observed image format, checksum/hash and source provenance.
Treat any model/region mismatch or unknown package signature as a stop
condition. Do not redistribute proprietary images or run an updater
merely to learn the commands. There is no defensible evidence yet
for cloning another Brother model's firmware onto the MFC-290C.

### Safe evidence record

| Item | What to preserve |
| --- | --- |
| Model and USB identity | Brother MFC-290C, reported `04f9:01fd` |
| Host OS and topology | Record the actual OS; USB location can change |
| USB interfaces and endpoints | Partial reported interface map; raw endpoint data unverified |
| USB device release (`bcdDevice`) | Record separately, if available |
| Firmware version from the machine's LCD | **Not yet observed** |
| Brother's published catalogue entry | **G**, dated 2011-09-08; not proof of installed firmware |
| Update or maintenance actions | **None necessary for read-only identification** |

Do not publish individual serial numbers, fax numbers or media filenames.
A shared device ID does not establish identical vendor commands
across firmware versions.

Before exploring USB commands, preserve the original print/scan/fax
capture source and interface/endpoint context in
[the USB command evidence ledger](brother-mfc290c.md). No firmware update,
maintenance-mode entry, reset, print job, scan movement or fax dialing
is required for the above investigation.


## Candidate instruction sets: cross-check against the QEMU fork (2026-11-22)

**Hardware proof remains unchanged.** Brother SM-FAX098 §2.1 assigns the
MFC-250C/MFC-290C tier a **128 MHz RISC CPU**; §3.3.1 depicts the main ASIC,
SDRAM, ROM, EEPROM and peripherals. The main-board family designation
`B53K961` is a Brother parts-list **board symbol**, not a semiconductor
manufacturer/model or a CPU instruction-set identifier. Parts-list variants
include MFC-290C (US early-production `LT0288052`; general `LT0288051`)
and other lower-tier 2008 machines; their common family symbol is an
opportunity for *board-marking comparison*, not proof of identical firmware.

Two independently documented **older and different Brother machines** offer
distinct hypotheses: the MFC-8220 service manual says its ASIC incorporates
a **Fujitsu 32-bit SPARClite** core; the HL-2700CN service manual identifies
a **Toshiba TMPR4955BFG-300 MIPS64** CPU. Neither part was identified on the
MFC-290C, and the original 128 MHz rating cannot distinguish these ISAs.

The existing WHP QEMU fork, checked on 2026-11-22, has these *generic*
system-emulation targets and instruction-source candidates:

| Hypothesis | Existing QEMU source/config | What it does **not** establish |
| --- | --- | --- |
| SPARC-family / SPARClite | `target/sparc/`, `configs/targets/sparc-softmmu.mak`; `disas/sparc.c` lists SPARClite-specific `divscc`, `scan` and extended FPop mnemonics | A compatible SPARClite CPU model, execution of those extensions, or any SPARC silicon in the MFC-290C |
| Older MIPS32/MIPS64 | `target/mips/`, `configs/targets/mips-softmmu.mak` and `mipsel-softmmu.mak` (distinct byte order) | Toshiba's processor, 32/64-bit mode, or endian selection for the MFC-290C |
| Pre-2008 32-bit Arm | `target/arm/`, `configs/targets/arm-softmmu.mak` | An Arm core, Thumb mode or board peripheral model for the MFC-290C |
| SuperH-4 | `target/sh4/`, `configs/targets/sh4-softmmu.mak` | Any Brother-specific reason to prefer SH-4 |

**Specific negative result:** `disas/sparc.c` includes SPARClite opcode
descriptions, but the fork's `target/sparc/translate.c` and
`target/sparc/insns.decode` do not name `sparclite`, `divscc`, `scan`
or the `efadd*` extension family. A disassembler *table entry* is not
proof that QEMU's TCG translator implements the corresponding instruction.
This is an **inspection result**, not an executed translation regression;
individual behavior would need separate tests if an authentic SPARClite
firmware/core identification is established.

For the original 2008 silicon, RISC-V and AArch64 remain chronologically
ineligible even though modern QEMU supports them. This does not disqualify
older 32-bit Arm or pre-2008 MIPS/SPARC families.

**Gate before opcode analysis:** obtain an authenticated, lawfully available,
read-only firmware *image* or clear processor package marking. Record model,
board/region, source, SHA-256, image versus updater/container distinction,
actual file layout, code segment offset, load address, byte order and CPU
mode. Do not apply `qemu-system-*` or an opcode-density heuristic blindly
to a compressed/encrypted updater, random data or an arbitrary memory
address. A plausible disassembly alone is weak: confirm reset/control-flow
structure, multiple real code paths and memory/peripheral references.
Only after ISA identification should QEMU execution use the **selected
core**, memory map, reset vector and real peripheral behavior; QEMU's
off-the-shelf SPARC/MIPS/Arm/SH-4 boards are not an MFC-290C machine model.
No firmware update, maintenance request, EEPROM write or host USB command
is justified by this investigation.

Sources and inspectable comparisons:
- Brother SM-FAX098 2008 §2.1 and §3.3.1:
  https://manualmachine.com/brother/dcp145c/18240960-service-manual/
- Brother PL-FAX154, List 8 (main-PCB symbols and assembly variants):
  https://documents.cdn.ifixit.com/ViQd2JUqMFeULuyJ.pdf
- Brother MFC-8220 SPARClite service manual:
  https://www.manualzz.com/doc/6272216/brother-mfc-8220-multifunction-printer-service-manual
- Brother HL-2700CN Toshiba MIPS64 service manual:
  https://www.manualslib.com/manual/745476/Brother-Hl-2700cn.html?page=98
- QEMU fork, SPARClite disassembler tables:
  https://github.com/retrochristian5000/QEMU/blob/master/disas/sparc.c
- QEMU fork, SPARC execution/decoding sources:
  https://github.com/retrochristian5000/QEMU/blob/master/target/sparc/translate.c
  https://github.com/retrochristian5000/QEMU/blob/master/target/sparc/insns.decode
