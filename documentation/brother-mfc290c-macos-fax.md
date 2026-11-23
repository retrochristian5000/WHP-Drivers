# Brother MFC-290C — macOS fax integration and driver boundaries

**Research state (2026-11-23): design only.** This is a focused continuation of [MFC-290C hardware/protocol evidence](brother-mfc290c.md) and [PC-FAX / modem compatibility work](brother-mfc290c-fax-modem-compatibility.md). No actual Brother USB PC-FAX command, native fax submission, received-PC fax, SIP/VoIP transport, or host-side dialing is implemented. Do not install device drivers or create a live fax queue as an automatic test.

## Supported facts and limits

| Fact | Strength and implication |
| --- | --- |
| Apple documents **sending through a fax-capable multifunction printer via the Print dialog**, and a **separate fax queue** in Printers & Scanners, including current Mac help | Apple platform capability; **not proof** that the MFC-290C is supported by a current release |
| Apple/CUPS defines `CUPS_PRINTER_FAX = 0x40000` | Fax *queue classification*, not a manufacturer-specific device code, USB class, service registration mechanism, or transmission receipt |
| Brother's MFC-290C Macintosh PC-FAX instructions specify **Brother CUPS printer driver**, `Send Fax` application selection, `Facsimile` output, destination numbers, then `Print` | Original Brother-supported host path; Brother explicitly notes Apple-branded applications on macOS **10.14** do not support its Send Fax feature and suggests PDF + third-party application. It does not promise modern Apple Silicon compatibility |
| Brother's current **MFC-290C** listed Mac OS support ends at **macOS 10.15** | Last *listed* OS, not an asserted hardware limit. Preserve historical/modern separate version gates; do not assume contemporary Brother binaries work, or that every listed version supports PC-FAX |
| Brother model supports **monochrome PC-FAX send only**, not computer-based fax reception; standalone LINE fax receive is separate | Never advertise receive capability because Apple/macOS exposes fax destinations or the printer can physically receive |
| Existing user-reported file called **PDD** contains `*Resolution 200dpi: ""`, `*InstalledMemory Memory: "8,72,8"`, `*PrinterCaps: "33292"` | PPD-like observations, source file not authenticated. Do not convert them to USB commands, macOS device status, installed RAM, or CUPS capability flags |

**Explicit flag-namespace separation:** User's Brother `PrinterCaps: "33292"` equals `0x820C` if interpreted as a decimal integer. Apple's `CUPS_PRINTER_FAX` is `0x40000`. They come from *different data models and value spaces*; neither implies anything about the other. Do not decode Brother's unverified `PrinterCaps` using CUPS bit definitions.

## What legacy macOS fax did and did not do

- **User interface:** Brother's legacy CUPS-driver integration added Fax-related print settings and a destination entry point. Apple documents Print-dialog fax and fax queues where the printer's software supports them. A selected fax destination is not automatically a modem or a generic USB serial port.
- **CUPS spooler:** Manages job delivery and status; filters render from PDF/PS/raster into the destination format; backends transport to the device. CUPS separates rendering from device I/O. Any *Brother-specific fax job envelope*, destination encoding, transfer endpoint, acknowledgement and error mapping must be recovered independently from model-authenticated driver evidence.
- **Fax hardware:** Actual PSTN/fax signaling occurs in the MFC-290C's own modem through the rear `LINE` jack, potentially using compatible analog phone service such as Spectrum Voice. The computer USB path is not an analog modem connection.
- **Apple historical API:** `NSPrintFaxReceiverNumbers`, `NSPrintFaxModem` and other fax-oriented `NSPrintInfo` keys are **deprecated**; do not build a new macOS fax implementation around them solely to imitate the old software.
- **USB:** For this model `04f9:01fd`, user-observed interface 00 is printer class `07/01/02`, interface 01 vendor-specific `FF/FF/FF`, and interface 02 mass storage `08/06/50`. None authenticates where Brother PC-FAX jobs are sent, or shows native IPP-over-USB. The existing native read-only IOKit probe remains the first discovery gate.

## Two Mac-side integration approaches — make a measured choice

### A. Historic CUPS fax queue/filter/backend adapter (compatibility path)

```text
App Print / fax destination -> CUPS job + fax destination metadata
      -> PDF or raster renderer (no USB)
      -> Brother outbound fax-job converter (needs authenticated format)
      -> USB transport component (needs verified endpoint/opcodes)
      -> MFC-290C internal fax + LINE
```

Use this to recover and compare the **original Brother macOS behavior** and to build an offline compatibility fixture. CUPS documentation defines a filter/backend interface and provides `STATE:`/backchannel reporting. On macOS, CUPS filters/backends run under **sandbox constraints**: CUPS docs allow USB device access in backends, **not filters**; do not put USB I/O inside the renderer. Sandbox and installation behavior depend on the actual OS version; do not assume a historical PPD/filter package can be installed on modern macOS.

**Durability risk:** PPD/classic-driver mechanisms are legacy/deprecated. New code should not rely solely on undocumented Apple fax UI properties or old CUPS driver deployment.

### B. Local IPP FaxOut service / Printer Application-style facade (modern interoperability candidate)

```text
Mac fax-aware client / optional print UI
        -> local IPP FaxOut-compatible service (future, only if supported)
        -> host-neutral outbound fax job + truthful job-state engine
        -> authenticated Brother PC-FAX conversion and USB transport
        -> MFC-290C LINE
```

The Printer Working Group has standardized **IPP FaxOut (PWG 5100.15)**. OpenPrinting documents **driverless-fax** and a Printer Application pattern: a local service can advertise a legacy device as a modern **IPP service** even when the underlying hardware has only a legacy proprietary USB protocol. This architecture could be reusable on Linux and macOS if the host's printing/fax client accepts and presents the advertised fax service. **That macOS end-to-end compatibility is not established by the standard alone**, so test discovery, fax queue creation, recipient handling and actual OS behavior first.

**Do not falsely claim IPP-over-USB or AirPrint in MFC-290C firmware.** A local IPP server would **translate IPP FaxOut jobs to verified Brother USB commands**; it is not a transparent `ipp-usb` proxy. `ipp-usb` applies to devices that really implement IPP-over-USB; that has not been observed on our MFC-290C.

### Decision

Preserve **one device-facing Brother outbound backend** and expose the smallest necessary host adapter. Priority: (1) authentic PC-FAX job format, (2) offline CUPS/fax job conversion and status model, (3) mock local IPP FaxOut discovery/job test if macOS supports the target workflow, (4) optional legacy CUPS fax queue adapter only if needed. Do not write two separate fax engines.

## Status accuracy and privacy

Keep at least these distinct conceptual states: `queued`, `rendered`, `submitted_to_backend`, `accepted_by_device`, `dialing`, `negotiating`, `delivered`, `failed`, `cancelled`, `unknown`. These are **project-side conceptual states**, not authenticated Brother codes. A successfully completed CUPS/IPP queue operation **cannot alone** certify that a remote fax machine received a complete document. A legitimate `delivered` event requires actual device-side session/result evidence.

- Destination telephone numbers and document contents are sensitive: never log raw telephone numbers, recipient names or unredacted fax content by default; use masked identifiers and sanitized offline fixtures.
- A network-visible fax service exposes an ability to **place outbound telephone calls**. Default to local-only/bind-loopback mock service in tests, with authorization/authentication and explicit operator approval before enabling real dialing or network sharing.
- Respect cancellation, device removal, retries, duplicates and timeouts. Do not retransmit automatically after uncertain outcomes, because it might duplicate a fax or incur telephone charges.
- `BRHOOK.DLL`/`SetBrHook`/`USER32!CallNextHookEx` are Windows event-hook research, **not Mac dependencies**. Status UI on macOS should consume a host-neutral event stream and use native notifications; printer availability via IOKit and queue state via CUPS/IPP remain different data sources from actual fax modem results.

## Non-invasive macOS investigation

Run these only on the user's own Mac, read-only, *without fax phone numbers or real submission*:

```sh
# Existing printer/fax queue and backend inventory, if present:
lpstat -v
lpstat -p
# List capabilities for a known queue only (replace placeholder with actual queue name):
lpoptions -p QUEUE_NAME -l
# Installed Brother package references, no package modifications:
pkgutil --pkgs | grep -i brother
```

Interpret `lpstat` and `lpoptions` only as queue/driver evidence. A queue URI, a `fax` keyword or a generated PPD does not authenticate Brother fax commands. Before exploring hardware, reuse the repository's existing `./build.sh probe-brother-mfc290c-macos` read-only IOKit/CUPS observer and verify no other OS process owns the interface. Avoid `lp` to a real fax destination, `lpadmin`, `cupsctl`, driver install, service registration, device reset, and USB OUT payloads in the initial phase.

## Validated offline acceptance gates

| Gate | Evidence needed | State as of 2026-11-23 |
| --- | --- | --- |
| Mac fax support research | Apple current help, CUPS `CUPS_PRINTER_FAX`, Brother MFC-290C original CUPS fax workflow and OS list | **Documented** |
| Authentic legacy file and filter | Actual Mac Brother PDD/PPD, filter/back-end identities, supported architecture, version, conversion rules | **Not inspected** |
| Native USB fax transport | Sanitized model-specific USB descriptors and verified Brother PC-FAX job stream, status readback | **Unknown** |
| CUPS offline rendering | Mock job with monochrome pages, exactly validated recipient metadata handling, no real dialing, robust cancellation | **Not implemented** |
| Local IPP FaxOut service | Isolated mock `ipp/faxout` service, capability discovery by target macOS, authenticated recipient/queue, error and cancellation tests | **Not implemented** |
| Connected fax result | Authorized real hardware/LINE controlled fax send and remote acknowledgement | **Not attempted** |

**Economic rule:** Prefer one backend used on Windows and Mac; do not fund two proprietary driver implementations, a separate fax modem, or a new kernel extension without evidence the cheaper user-space route is inadequate. Include installation maintenance, document privacy controls, service power usage and repeat-fax charges in any later cost comparison.

## Primary references

- Apple, [Set up a scanner or fax on Mac](https://support.apple.com/en-euro/guide/mac-help/mh28039/27/mac/27)
- Apple, [Send a fax using a multifunction printer](https://support.apple.com/en-za/guide/mac-help/mchlp2449/27/mac/27)
- Apple, [Deprecated printing keys](https://developer.apple.com/documentation/appkit/deprecated-printing-keys)
- Apple CUPS, [`CUPS_PRINTER_FAX` definition](https://github.com/apple/cups/blob/master/cups/cups.h)
- CUPS, [Filter and backend programming](https://openprinting.github.io/cups/doc/api-filter.html)
- CUPS, [Programming manual](https://openprinting.github.io/cups/doc/cupspm.html)
- Brother, [MFC-290C PC-FAX for Macintosh](https://help.brother-usa.com/app/answers/detail/a_id/58762/~/send-a-pc-fax---windows-or-macintosh)
- Brother, [MFC-290C OS support](https://support.brother.com/g/b/oslist.aspx?c=us&lang=en&prod=mfc290c_all)
- PWG, [IPP FaxOut (5100.15-2014)](https://www.pwg.org/standards.html)
- OpenPrinting, [Printer Applications](https://openprinting.github.io/upcoming-technologies/01-printer-application)
- OpenPrinting, [driverless-fax support](https://github.com/OpenPrinting/cups-filters/releases)
- OpenPrinting, [ipp-usb requirements](https://github.com/OpenPrinting/ipp-usb)
