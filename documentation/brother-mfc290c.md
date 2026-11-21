# Brother MFC-290C — USB multifunction driver evidence and boundaries

**Status:** portable USB identity/model-label decoders, synthetic host tests, and read-only macOS IORegistry/CUPS observers. **Not an installable driver.**

## Identified hardware

- Model: Brother MFC-290C, color inkjet printer, flatbed/ADF scanner and standalone analog fax.
- Computer connection: USB 2.0 **Full-Speed** (maximum signaling rate 12 Mbit/s), not 480 Mbit/s High-Speed. USB A-to-B cable; Brother recommends at most 2 m.
- Reported USB vendor/product pair: `04f9:01fd`. Cross-checked against independent scanner identification and Linux user reports. Confirm the actual attached unit's device descriptor before binding.
- Brother documents printing, scanning, card access, and **sending** PC-FAX. Standalone incoming fax is supported, but **PC-FAX receive is not available** on MFC-290C. A telephone line connects to the machine's LINE jack, not to a PC serial/modem port.
- Brother lists legacy Windows and Mac software and maintains Linux downloads. Existing third-party scanner software also supports this model. The aim is portable *new* support where feasible, not an unsupported claim that no drivers exist.

## Code contract

`drivers/usb/brother/mfc290c.{h,c}` parses a caller-owned USB device descriptor, validates its fixed 18-byte framing and checks the exact VID/PID. A match exposes raw descriptor identity fields; it **does not** imply a usable interface, endpoint, negotiated speed, driver binding, or fax connection. The feature mask describes the physical model and documented host features, not implemented operations. It deliberately excludes PC-FAX reception.

- USB controller PCI inventory (`drivers/usb/pci_hci.*`) does **not** enumerate attached USB devices; `WHPPROBE.EXE` must not report MFC-290C from an EHCI/OHCI/UHCI/xHCI controller alone.
- Do not assume the device is USB printer class or USB communications class without actual configuration/interface/endpoint descriptors. Do not send vendor requests or reset the device based on this identity alone.
- Do not confuse standalone ITU-T fax modem signaling with Hayes AT commands on a PC COM port. Any future PC-FAX submission protocol requires independently captured, authorized device evidence.
- USB transport, print stream encoding, scanner commands, PC-FAX sending, OS install packages, and real-hardware behavior remain **unimplemented**. No firmware updates, ink movement, phone calls, or USB requests occur in the identity tests or macOS observer.

## Firmware, flash ROM and service-mode boundary (2026-11-21)

Brother confirms an MFC-290C firmware update tool and a published firmware
release marked **G** (2011-09-08). The 2008 family service manual distinguishes
**main-PCB flash firmware**, **model/region/calibration EEPROM** and
**firmware switches (WSW)**. These are not synonymous with USB command
types, and a release letter is not proof of the firmware installed on this
particular printer. Brother also documents a physical-LCD firmware-version
check using the `*` and `#` keys together. The macOS USB `bcdDevice`
field reports **USB device revision**, not that Brother firmware letter.

The maintenance USB printer driver described in the service manual is a
separate software/update context from ordinary print, scan and PC-FAX
operation. No authenticated MFC-290C flash/update command, image format,
bootloader endpoint, device-side command dispatcher or firmware binary has
yet been recovered. Do **not** replay another Brother model's update
sequence or enter maintenance mode merely to inspect the firmware.

For the official source links, evidence categories, safe LCD observation
and offline firmware-analysis plan, see the
[dedicated MFC-290C firmware evidence page](brother-mfc290c-firmware.md).
That page is authoritative for this firmware investigation; this ledger
retains the shared USB, printing, scanning and PC-FAX integration boundaries.

## macOS USB Location ID observation (2026-11-21)

A connected MFC-290C was reported by macOS with **Location ID
`0x00120000`**. This is an **observed topology value**, not a model
identifier, serial number, fixed USB endpoint or driver-binding requirement.
The macOS version, controller, hub chain and physical connector for this
observation have not been recorded; do not infer a specific physical port from
the hexadecimal digits alone.

Apple's original IOUSBFamily `IOUSBLib.h` documents
`GetLocationID` as returning a 32-bit identifier for the USB device's
position in the system topology. It is normally stable across reboots while
topology remains unchanged, but may differ after a cable/hub/port/controller
change. Apple's ImageCaptureCore also exposes an `ICDevice.usbLocationID`
property, which may be useful for **future** scanner-registry correlation;
no ICA device matching has yet been implemented.

The macOS probe now accepts a **diagnostic-only** comparison:

```sh
./build.sh probe-brother-mfc290c-macos --location-hint 0x00120000
```

The optional argument requires exactly eight hexadecimal digits preceded by
`0x` or `0X`. It is not saved in a configuration file and never restricts
USB enumeration. All devices with matching Brother **VID/PID** are still
reported. Where macOS exposes a location ID, each match is annotated
`locationHint=same` or `locationHint=different`; if the property is
missing it reports `locationHint=unavailable`. An invalid hint fails before
registry access. A moved device is **still recognized** if its VID/PID matches.

The portable parser `drivers/usb/brother/mfc290c_location.{h,c}` is covered
by synthetic checks for valid 32-bit values, rejected malformed hints,
unchanged output on failure, and stable VID/PID matching after a simulated
location change. Comparing a location cannot establish printer, scanner
or fax endpoints, protocols, print queues, USB speed, or functional driver
support. Never hard-code this observation into a Windows, DOS or macOS driver.

References:
- Apple IOUSBFamily `GetLocationID` contract:
  https://github.com/apple-oss-distributions/IOUSBFamily/blob/main/IOUSBFamily/Headers/IOUSBLib.h
- Apple ImageCaptureCore `ICDevice.usbLocationID`:
  https://developer.apple.com/documentation/imagecapturecore/icdevice/usblocationid

## macOS Intel / Apple Silicon architecture audit (2026-11-21)

The MFC-290C USB Full-Speed protocol is the same regardless of its macOS
host CPU. PowerPC, x86_64 and AArch64 are host code architectures, **not**
USB endpoint numbers, printer command families or the device firmware ISA.
Keep the existing explicit byte decoding, not host-ABI struct casts.

The IOKit/CUPS diagnostic reports the executing process ISA and pointer
width plus sysctl.proc_translated (Rosetta) status when available.
The read-only --architecture-only option skips both USB registry and CUPS
queries. A rosetta=unknown result is not proof of native execution.

Build mode WHP_MFC290C_MACOS_ARCH accepts native (default), arm64, x86_64
or universal2. The build passes explicit -arch compiler arguments and
verifies Mach-O slices using lipo; universal2 requires both arm64 and
x86_64. The special arm64e kernel-extension ABI is deliberately rejected
for this user-space probe; it is not a DriverKit extension or a kext.

Example (on macOS):

    WHP_MFC290C_MACOS_ARCH=universal2 ./build build-brother-mfc290c-macos
    lipo -archs .build/brother-mfc290c-macos/mfc290c-probe
    ./.build/brother-mfc290c-macos/mfc290c-probe --architecture-only

The pinned macOS 15 Apple Silicon CI runner compiles both slices and
executes the arm64 slice only. Intel runtime, historical PowerPC builds,
the physical MFC-290C, USB transfers, printing, scanning, PC-FAX and
signed USBDriverKit entitlement/installation are not yet tested.

References: Apple Universal Binary, Rosetta and USBDriverKit documentation:
- https://developer.apple.com/documentation/apple-silicon/building-a-universal-macos-binary
- https://developer.apple.com/documentation/apple-silicon/about-the-rosetta-translation-environment
- https://developer.apple.com/documentation/kernel/implementing_drivers_system_extensions_and_kexts
- https://developer.apple.com/documentation/usbdriverkit

## Native macOS USB evidence (IOKit)

macOS is an **independent target**, not a Windows 95 or DOS compatibility
wrapper. The dedicated source `tools/macos/mfc290c_probe.c` enumerates IOKit
`IOUSBHostDevice` registry services, falling back to the historical
`IOUSBDevice` class when the first search has no matching Brother device.
It verifies numeric `idVendor=04f9` and `idProduct=01fd` and prints a
reported `bcdDevice` revision, USB location ID and any matching child
interface-class/number/subclass/protocol properties that are visible. Missing
properties appear as unknown rather than guessed values. Enumeration is bounded
to prevent runaway registry walks; the tool does **not** output serial numbers.

The macOS program reads the host's registry only. It never opens, claims,
reconfigures, resets, or sends transfers to the device. These **registry
observations are not raw USB descriptors** and cannot establish endpoint
addresses, active alternate settings, negotiated USB speed, scanning protocol,
print encoding or PC-FAX command semantics. If no matching registry service is
visible, that does not prove the hardware is disconnected or unsupported.

```sh
WHP_DRIVER_PROFILE=brother-mfc290c ./build.sh plan
WHP_DRIVER_PROFILE=brother-mfc290c ./build.sh test-fast
WHP_DRIVER_PROFILE=brother-mfc290c ./build.sh build
./build.sh probe-brother-mfc290c-macos
```

The `build` profile runs the cross-platform host test and compiles the IOKit
probe when the **build host** is macOS. The explicit `probe-brother-mfc290c-macos`
command then compiles and **runs** the read-only physical registry observation;
it requires macOS with its native C compiler and IOKit/CoreFoundation SDK.
Both use a separate private output directory under
`.build/brother-mfc290c-macos/`. No LLVM bootstrap, old Windows DDK,
real-mode DOS driver or kernel extension is used. Modern macOS implementation
should separately evaluate user-space frameworks versus USBDriverKit and their
actual security/entitlement requirements before binding device interfaces.

Observations from macOS, Linux, Windows or DOS can disagree because their host
stacks enumerate and expose different metadata. Record the source, machine,
operating-system version and raw/report-derived status of each value; never
silently choose the "most accurate" OS as authoritative without hardware
documentation and descriptor evidence.

## macOS print and scan driver logic (evidence versus implementation)

Brother's original Mac software used a **CUPS printer-driver path** and an
**Image Capture Architecture (ICA) scanner path**. Brother's MFC-290C Mac OS X
10.6 instructions specifically state that its CUPS Printer Driver package
contained the ICA Scanner Driver. These are separate operating-system
interfaces, not evidence that the USB device presents separate printer and
scanner class interfaces. We must obtain USB configuration/endpoint descriptors
before selecting any USB interface or issuing transfers.

The macOS observer now reports **two independent evidence layers**:

1. **IOKit USB registry:** exact numeric USB VID/PID `04f9:01fd`, visible
   interface fields and descendant service-class/`CFBundleIdentifier` hints.
   A bundle hint is **not proof** of the active Brother print/scan implementation.
2. **CUPS destinations:** the standard `cupsGetDests` metadata snapshot and
   number of queues whose `printer-make-and-model` text contains distinct
   `Brother` and `MFC-290C`/`MFC290C` tokens. A queue model-label candidate
   is **not** a verified USB device, a working installed filter, a printer
   protocol, or a confirmed connected physical unit. Other model variants
   with appended letters/numbers are rejected. Missing labels remain unknown.
   Neither queue names nor device URIs nor serial numbers are printed.

The CUPS model-label check is an OS-neutral C11 function in
`drivers/usb/brother/mfc290c_model_match.{c,h}`, covered by synthetic positive,
negative, neighboring-model, NULL and overlength tests. The CUPS-specific
adapter is `tools/macos/mfc290c_cups.{c,h}`. It **only queries** the print
scheduler; it does not submit jobs, configure queues, install a PPD, open a
USB interface, or modify driver settings. The native macOS build links the
system `IOKit`, `CoreFoundation` and `cups` libraries. Querying CUPS may
contact the local print scheduler; the caller performs no direct USB transfers.

**Implementation design:** keep transport and Brother wire protocols portable,
then adapt them separately to (a) a macOS print service connected to CUPS/IPP,
(b) ImageCaptureCore/ICA scanning, and (c) PC-FAX sending. Do not assume this
USB-only legacy device supports native AirPrint or IPP Everywhere; a future
printer application could expose an IPP-compatible interface only *after*
its Brother-specific print stream is understood. Classic PPD-based CUPS filters
are deprecated, so new code should not depend on their permanent availability.
Do not package a USBDriverKit extension merely to inspect a registry:
USBDriverKit binding, signing and entitlements apply only if an actual
driver extension is needed. Avoid installing unsigned kernel extensions or
copying proprietary Brother binaries into our source tree.

**Unverified:** native macOS SDK build/CI result, presence and content of
actual MFC-290C registry services, CUPS destination setup, historical Brother
filter behavior, ICA scanner availability on modern macOS, device endpoints,
print stream, scanner commands, fax protocol, and real printing or scanning.
A clean macOS CI runner has no target printer, so successful compilation and
empty observations cannot certify hardware support.

## USB command recovery: standards, interfaces, and unknowns (2026-11-21)

This is an **evidence inventory**, not a command sender. Neither Brother's
manuals, the existing macOS IORegistry output, nor our synthetic tests provide
an authenticated MFC-290C USB configuration descriptor or device-side
command transcript yet. The model-level `04f9:01fd` identity and previously
observed macOS `locationID=0x00120000` do not establish USB interface
classes, endpoint addresses, or fax command bytes.

### Standard USB SETUP requests (protocol-defined, not model-tested)

The host's USB stack manages standard enumeration. Our host-neutral decoder
can recognize the following **captured eight-byte setup records**, in
little-endian USB SETUP field order. It does **not** send the requests:

| Request | `bmRequestType` | `bRequest` | Evidence/conditions |
| --- | --- | --- | --- |
| `GET_DESCRIPTOR` | `0x80` | `0x06` | Standard read of device/configuration/other descriptors |
| `GET_CONFIGURATION` | `0x80` | `0x08` | Standard read of current configuration |
| `GET_INTERFACE` | `0x81` | `0x0A` | Standard read of alternate setting for a real interface |
| `GET_STATUS` | `0x80`/`0x81`/`0x82` | `0x00` | Standard device/interface/endpoint status read |

The USB-IF **Printer Class 1.1** specification defines additional requests
**only for an interface whose actual `bInterfaceClass` is `0x07`**:

| Printer-class request | `bmRequestType` | `bRequest` | Classification |
| --- | --- | --- | --- |
| `GET_DEVICE_ID` | `0xA1` | `0x00` | Candidate IEEE 1284 identification read |
| `GET_PORT_STATUS` | `0xA1` | `0x01` | Candidate printer status read |
| `SOFT_RESET` | `0x21` | `0x02` | **State-changing**; not for discovery |
| Historical reset recipient variant | `0x23` | `0x02` | **State-changing**; USB-IF notes prior 1.0 discrepancy |

The printer-class `GET_DEVICE_ID` request uses its `wValue` and `wIndex`
fields to select configuration/interface/alternate setting; these must be
derived from actual hardware descriptors. `GET_PORT_STATUS` returns a
one-byte parallel-printer-compatible status if implemented. A printer-class
interface typically uses Bulk OUT for print data and may have Bulk IN.
**None of these provide the MFC-290C print raster language**, and no claim
is made that the MFC-290C accepts any particular class-specific request.

### Scanner and PC-FAX: separate investigation, not invented opcodes

- **Scanner lead:** third-party Windows device/driver listings identify
  `USB\\VID_04F9&PID_01FD&MI_01` for an MFC-290C scanner driver.
  This is a **corroborating software binding record**, not a raw USB
  descriptor or verified class/endpoint map. It suggests investigation of
  interface 1, **not a rule that interface 1 always means scanning**.
- **Scanner comparative prior art:** the open-source `codyps/brother2`
  project documents ASCII-like request packets starting with `0x1b` and
  operation letters on *other* Brother scanners (e.g., MFC-7820N).
  Their request letters, framing, endpoint directions, and responses
  are **unverified for the MFC-290C** and must not be transmitted as-is.
- **PC-FAX:** Brother publishes a separate Linux PC-FAX package
  (`brmfcfaxdrv`) for this model. The historical macOS workflow exposed
  sending through a CUPS print-dialog fax option. **Neither establishes
  whether PC-FAX uses a dedicated USB interface or shares a printer data
  endpoint**. The actual fax job headers, number formatting, command bytes,
  and USB transfer framing are not recovered. Standalone fax protocols
  over the telephone line must not be treated as USB control requests.
- **Printing:** Brother publishes model-specific LPR, CUPSwrapper and
  CUPSwrapper source packages. These are candidates for legitimate *static
  inspection*, but mere package availability does not prove that the
  published wrapper source contains the low-level raster/proprietary USB
  protocol.

### Offline decoder, verification, and safe next evidence

`drivers/usb/brother/mfc290c_usb_commands.{h,c}` classifies a **captured**
eight-byte USB SETUP record when given the target interface class from a
separately verified descriptor. Unknown interfaces must be marked `0xFF`.
The classifier reports standard reads, conditional printer-class reads,
printer resets, host-to-device/state-changing requests, or unknown traffic;
it never emits a USB command. It also labels **possible** printer Bulk IN
and Bulk OUT endpoints only if supplied with actual endpoint attributes
and printer-class interface metadata. It deliberately **does not infer
scanner or fax traffic** from endpoint numbers, the macOS Location ID,
Windows `MI_01`, or vendor request values.

The synthetic tests `tests/brother-mfc290c-usb-commands.c` cover all standard
request names, both reset variants, malformed packet lengths, output
unchanged on invalid input, vendor-specific/unknown traffic, class-unknown
fallback, and printer Bulk IN/OUT conditional labels. Run
`./build test-brother-mfc290c` without any USB permissions or printer.

To recover Brother's *actual* commands, preserve (a) a sanitized full
configuration/interface/endpoint descriptor for a physically connected
`04f9:01fd`, (b) exact host OS and installed driver versions, (c) lawful,
authorized captures or static driver analysis for **one function at a time**,
and (d) both the raw eight-byte SETUP headers and transfer direction/endpoint
context. Begin with already-observed, read-only discovery and avoid
unreviewed control OUT transfers, soft resets, scan movement, print jobs,
fax dialing, or firmware updates. The host OS may already own USB
interfaces; never forcibly seize them for a probe.

Sources:
- USB-IF, *USB Device Class Definition for Printing Devices*, v1.1:
  https://www.usb.org/sites/default/files/usbprint11a021811.pdf
- Microsoft, USB printer driver binding:
  https://learn.microsoft.com/en-us/windows-hardware/drivers/print/printer-connected-to-a-usb-port
- Third-party model scanner MI_01 record (unverified independently):
  https://www.drivermax.com/Brother-MFC-290C-Hamrick-Software-USB-Vid-04f9-Pid-01fd-MI-01-1_0_0_90-2006-08-21-4336985-driver.htm
- Other-Brother-model scanner protocol research, **not MFC-290C**:
  https://github.com/codyps/brother2/blob/master/PROTO
- Brother MFC-290C Linux downloads (LPR, CUPS wrapper/source, scanner, PC-FAX):
  https://support.brother.com/g/b/downloadlist.aspx?c=us_ot&lang=en&os=128&prod=mfc290c_all

## Related Brother fax family: evidence and command-recovery controls (2026-11-21)

**Family evidence is not a transferable USB command set.** Brother's June
2008 service manual **SM-FAX098** explicitly covers the MFC-250C, MFC-290C,
MFC-490CW, MFC-790CW, MFC-990CW and DCP-145C/165C/185C/385C/585CW. This
is a **documented service-manual cohort**, not proof of identical PCBs,
firmware, USB descriptors, protocol versions, or command byte sequences.

| Comparison model | Source-backed relationship | How to use it / limit |
| --- | --- | --- |
| **MFC-290C** | Target: USB 2.0 **Full-Speed**, 14,400-bps G3 fax, monochrome PC-FAX sending; no PC-FAX reception | Identity **04f9:01fd** is the only model USB binding currently documented by this driver. No recovered fax opcodes yet. |
| **MFC-250C** | **Nearest positive control**: same 2008 manual and lower-end USB Full-Speed/14,400-bps group; PC-FAX sending yes, PC-FAX reception excluded by Brother | Compare **authorized static driver data** and independently captured fax submissions. Its USB product identity, interfaces, and commands must be discovered independently; never bind it as an MFC-290C. |
| **MFC-490CW** | **Advanced positive/control contrast**: same manual, but 33,600-bps fax, USB Hi-Speed, wired/wireless networking and PC-FAX reception on supported non-Mac systems | Useful for separating common PC-FAX *sending* software from features not present on the MFC-290C. Neither the network driver nor fax-receive Class 2 path should be assumed to exist on the target. |
| **MFC-790CW / MFC-990CW** | Additional higher-feature machines in the same 2008 manual | Lower priority comparisons; different handset, display, memory, and connectivity options make naive firmware/transport equivalence unsafe. |
| **DCP-165C / DCP-185C** | **Nonfax negative controls** from the same 2008 manual: printing/scanning hardware families, but fax/telephone and PC-FAX functions not available | Distinguish *candidate* fax-only traffic from general Brother printing/scanning/maintenance traffic. Differences can also result from unrelated model-specific features. |
| **MFC-255CW / MFC-295CN / MFC-495CW** | Listed alongside MFC-250C/290C/490CW in Brother's PC-FAX support lists, but **not** in SM-FAX098 | Useful secondary *software-stack* candidates only. Do not silently assign them the 2008 service-manual hardware family or its USB properties. |

**A critical "Class 2" ambiguity:** SM-FAX098 lists **RX: Class 2** under
the *advanced models' PC-FAX receiving protocol*. This refers to the fax
software/protocol path. It is **not evidence** that MFC-290C exposes an
USB CDC ACM modem, that interface 1 is a fax modem, or that Class 2
AT commands can be submitted over its USB printer endpoint. The target
has **no documented PC-FAX receiving**, despite standalone line reception.

### Brother's Linux PC-FAX pipeline: software family, not yet USB opcodes

Brother's published Linux instructions describe a `BRFAX` print queue,
the `brpcfax` submission utility, a `filterBRFAX` filter, and
`/opt/brother/fax/brmfcfax/inf/brFAXrc` configuration. A generic USB
LPD example feeds the printer device node `/dev/usb/lp0`; this is
**host-side spooler wiring**, **not** proof that MFC-290C has a dedicated
fax USB interface, nor that fax and printing use the same endpoint.
Brother's `brmfcfaxdrv` package spans many generations; the existence
of a shared package does not prove identical device-side commands.

The next useful **static artifacts** are the installed filter's text
wrapper/configuration, documented command-line options, and an
authorized **non-transmitting** fax-job file generated by the relevant
driver version. If model-specific bytes can be independently established,
record their source package/version, path, and exact context **without**
redistributing Brother's proprietary driver binary. Do not execute a
submission with a telephone number or alter the machine during discovery.

### Cross-model packet-evidence acceptance rule

1. Establish each physical device's exact VID/PID, raw USB configuration,
   interface, alternate setting, endpoint direction/type and USB speed
   before comparing packet streams. The MFC-290C's observed macOS
   `locationID=0x00120000` is a *topology hint*, never a family ID.
2. Label every record by model, firmware, host OS, driver/filter version,
   transfer type and whether it was generated by **printing, scanning or
   PC-FAX sending**. Preserve an unmatched/unknown classification.
3. Compare a **verified PC-FAX send** sequence on MFC-250C and MFC-290C
   against corresponding print/scan sequences and a fax-less DCP control.
   Repeat on MFC-490CW to test which sequences persist across the tier
   change. No same-looking packet alone is proof of shared semantics.
4. Promote a candidate to a **model-specific known command** only with
   direct MFC-290C evidence of its framing, direction, endpoint context,
   and a defensible function mapping. For sensitive actions, require
   controlled authorization and hardware safeguards before any replay.
   Do **not** guess fax dialing/reset requests or test against live lines.

**Current result:** a ranked comparison family and specific software
artifacts to inspect; **zero new authenticated MFC-290C vendor command
bytes**. Accordingly, the existing offline USB classifier, fax-policy
tests, and host-specific discovery behavior remain unchanged.

References (source content, not a license to transplant firmware):
- Brother 2008 *Inkjet MFC/DCP Service Manual*, SM-FAX098,
  §§2.1.7 and 2.3-2.3.3 (all ten models; USB tiers; fax/PC-FAX matrix):
  https://manualmachine.com/brother/dcp145c/18240960-service-manual/
- Brother MFC-490CW official specifications (33.6 kbps, PC-FAX send/receive,
  MAC receive exclusion, wired/wireless support):
  https://support.brother.com/g/b/spec.aspx?c=us&lang=en&prod=mfc490cw_all
- Brother Software User's Guide, PC-FAX Receive exclusion naming
  MFC-250C and MFC-290C:
  https://support.brother.com/g/s/id/htmldoc/mfc/mfc-5895cw/gb/html/sug/chapter6_3.html
- Brother Linux PC-FAX CUPS workflow (general models/driver family):
  https://support.brother.com/g/b/faqend.aspx?c=us&faqid=faq00100716_000&lang=en&prod=mfcl6810dw_us
- Brother Linux PC-FAX LPD/LPRng examples (`BRFAX`, `filterBRFAX`,
  `brFAXrc`, `/dev/usb/lp0`, **not MFC-290C-specific USB evidence**):
  https://support.brother.com/g/b/faqend.aspx?c=us&faqid=faq00100558_000&lang=en&prod=mfcl2710dw_us_eu_as

## Fax capability boundary (2026-11-21)

The MFC-290C is an independent telephone-line fax terminal with a **14,400
bit/s maximum modem rate**. Brother's model-specific specification lists
**MH/MR/MMR** monochrome encoding, **JPEG** color fax encoding, up to
**170 pages** of ITU-T test-chart memory and up to **90 black-and-white
broadcast locations**. These last two figures are conditional capacity
ratings, **not input-document page-count or USB packet-size limits**.
Its standalone fax supports monochrome/color sending and standalone receiving.
Brother's **PC-FAX sending is black-and-white only**, and the original
PC-FAX **receiving is unavailable** on the MFC-290C. Neither the name "color
fax" nor the presence of a telephone modem establishes color PC-FAX.

The host-neutral files \`drivers/usb/brother/mfc290c_fax.{h,c}\` provide
strict **documented-versus-unsupported** classifications and a small PC-FAX
send-document preflight. The preflight accepts a nonempty monochrome page
count and rejects empty, color or unknown image modes. It does **not**
validate telephone numbers, raster encodings, call setup, live device state,
memory availability, fax negotiation, actual document contents, or a job's
ability to transmit. Its \`DOCUMENT_ACCEPTABLE\` result describes *only*
these static constraints. A documented fax mode returns
\`WHP_MFC290C_FAX_DOCUMENTED_ONLY\`, never "driver ready."

The model-specific fax test (\`tests/brother-mfc290c-fax.c\`) checks all six
separate modes, unsupported PC receive and color PC send, invalid inputs,
the existing \`mfc290c.h\` capability mask, and the distinction between
a memory *rating* and a hard job-size limit. The existing
\`./build test-brother-mfc290c\` target runs these tests on supported host
C compilers without a printer. The existing macOS/Linux CI matrix
covers the source; it cannot certify an attached fax line.

**macOS integration direction:** Brother historically exposed PC-FAX sending
as a **Send Fax / Facsimile** choice through the **Brother CUPS printer
driver**. A file in an application was submitted via this workflow with a
destination fax number. A working **analog telephone line connected to the
MFC-290C**, not a PC serial modem, remained required. Brother's modern
help page notes that its classic Fax option depends on a compatible
Brother CUPS driver, and indicates Apple application restrictions on macOS
10.14. This is evidence for how the original driver was integrated, not
proof that the same print dialog or binary works on current Apple Silicon.

The remaining implementation gates are: inspect authorized authentic
Brother host-driver data and/or record safe USB descriptor evidence; map
the exact printer-versus-PC-FAX submission protocol; determine original
monochrome fax document encoding and per-job metadata; design a macOS
submission adapter without causing unintended dialing; test under physical
fax-line safeguards. Do **not** guess vendor USB requests, invent T.30
signaling as a USB driver protocol, submit real calls from CI, or enable
PC-FAX receive merely because standalone reception works.

Sources:
- Brother MFC-290C product specifications:
  https://support.brother.com/g/b/spec.aspx?c=us&lang=en&prod=mfc290c_all
- Brother PC-FAX instructions, including MFC-290C and macOS:
  https://help.brother-usa.com/app/answers/detail/a_id/58762/~/send-a-pc-fax---windows-or-macintosh
- Brother PC-FAX Receive exclusion:
  https://support.brother.com/g/s/id/htmldoc/mfc/mfc-5895cw/gb/html/sug/chapter6_3.html
- MFC-290C model manuals:
  https://support.brother.com/g/b/manualtop.aspx?c=us&lang=en&prod=mfc290c_all

## Portability gates

| Target | Required proof before claiming driver support |
| --- | --- |
| DOS 6.x | USB host-controller access, enumeration and endpoint transport; a PCI inventory alone is insufficient. |
| Windows 95 RTM | A compatible USB host stack (RTM has no native USB support), plus the appropriate model/function driver ABI. Do not relabel a Windows 98 driver as RTM-compatible. |
| Windows 95 OSR2.x / Windows 98 | Verify exact release's USB stack and printer/scanner/fax function interfaces; do not silently borrow NT/WDM components. |
| Linux / BSD | Enumerate actual USB interfaces; integrate printing, scanning and PC-FAX with applicable user-space APIs only after protocol verification. Existing Brother Linux packages are separate prior art. |
| macOS (Intel / Apple Silicon) | **Native observation target added:** IOKit IORegistry inventory; current support is a read-only command-line probe, not printing/scanning/fax operation. Future drivers require the correct macOS frameworks, security/entitlements and verified USB protocols. | 
| Modern Windows | Use supported, architecture-correct driver and USB APIs; confirm host OS restrictions and actual function protocols. |

## Verification

Run `./build.sh test-brother-mfc290c` (or `./build test-brother-mfc290c`). It uses a native C11 host compiler to run synthetic **USB descriptor, CUPS model-label, fax policy, location-hint and offline USB command** tests; it does not need LLVM, OpenWatcom, USB permissions, or a printer. The targeted GitHub Actions job runs both tests on Linux and macOS and additionally compiles the IOKit/CUPS observer on macOS. A CI Mac without an attached MFC-290C cannot verify physical-device behavior.

## Primary and corroborating references

- Brother, *Quick Setup Guide*, 2008 (USB speed, cable, physical phone connections): https://download.brother.com/welcome/doc002385/cv_mfc290_usaeng_qsg.pdf
- Brother MFC-290C ICA scanning on Mac OS X 10.6: https://support.brother.com/g/b/faqend.aspx?c=as_ot&faqid=faq00000717_002&ftype3=1992&lang=en&prod=mfc290c_all
- CUPS Programming Manual, destination attributes: https://www.cups.org/doc/cupspm.html
- OpenPrinting, printer applications and legacy drivers: https://openprinting.github.io/cups/drivers.html
- Apple ImageCaptureCore: https://developer.apple.com/documentation/imagecapturecore
- Apple IOUSBHostDevice (IOKit/IOUSBHost): https://developer.apple.com/documentation/iousbhost/iousbhostdevice
- Apple USBDriverKit: https://developer.apple.com/documentation/usbdriverkit
- Brother OS compatibility listing: https://support.brother.com/g/b/oslist.aspx?c=us&lang=en&prod=mfc290c_all
- Brother MFC-290C manuals (User's Guide and Software User's Guide): https://support.brother.com/g/b/manualtop.aspx?c=us&lang=en&prod=mfc290c_all
- Brother MFC-290C specifications: https://support.brother.com/g/b/spec.aspx?c=us&lang=en&prod=mfc290c_all
- Brother PC-FAX receive exclusion: https://support.brother.com/g/s/id/htmldoc/mfc/mfc-5895cw/gb/html/sug/chapter6_3.html
- Brother Linux downloads: https://support.brother.com/g/b/downloadtop.aspx?c=us&lang=en&prod=mfc290c_all
- Independent USB ID corroboration: https://www.hamrick.com/es/vuescan/brother-mfc-290c-controlador.html and https://askubuntu.com/questions/151201/
