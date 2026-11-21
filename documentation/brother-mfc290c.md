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

Run `./build.sh test-brother-mfc290c` (or `./build test-brother-mfc290c`). It uses a native C11 host compiler to run synthetic **USB descriptor and CUPS model-label** tests; it does not need LLVM, OpenWatcom, USB permissions, or a printer. The targeted GitHub Actions job runs both tests on Linux and macOS and additionally compiles the IOKit/CUPS observer on macOS. A CI Mac without an attached MFC-290C cannot verify physical-device behavior.

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
