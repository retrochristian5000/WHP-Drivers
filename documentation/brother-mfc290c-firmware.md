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

## Three independent version observations

| Field | Source | Current evidence |
| --- | --- | --- |
| `installed_firmware_letter` | **Physical MFC-290C LCD** | **Unknown** until observed directly |
| `brother_published_release` | Brother MFC-290C firmware download listing | **G**, date **2011-09-08** |
| `usb_device_revision_bcd` | USB descriptor / macOS IOKit `bcdDevice` property | Reported by the device probe when available; **not equivalent to the firmware letter** |

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

The main-board firmware may contain dispatch routines for host transfers,
fax-job intake, scanner control, print streams, status/error reports, and
maintenance operations. The service manual shows these functions can
interact with stored configuration and calibration. **The structure,
entry points, implementation language, exact USB vendor opcodes, bootloader
layout, image format, cryptographic validation, flash memory map and
firmware-to-command compatibility remain unverified.** The existence of
a downloadable updater is not evidence for a particular command.

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

### Safe observation template

```text
Model: MFC-290C
USB VID:PID (observed): 04f9:01fd
Host and OS version: [record locally]
USB locationID (observed): [record locally; may move]
USB bcdDevice (observed): [record separately; not a firmware letter]
Firmware letter (on-machine LCD): [unknown until checked]
Brother catalogue release: G (2011-09-08; not installed-version proof)
Update/maintenance actions taken: none
```

Before exploring USB commands, preserve the original print/scan/fax
capture source and interface/endpoint context in
[the USB command evidence ledger](brother-mfc290c.md). No firmware update,
maintenance-mode entry, reset, print job, scan movement or fax dialing
is required for the above investigation.
