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

## macOS installation and distribution plan (2026-11-23)

**Present boundary:** The repository currently contains a read-only native `mfc290c-probe`, USB identity/classification code, synthetic fax-policy checks and documentation. It **does not have an installable, functioning MFC-290C print/scan/fax transport**. This section is a reproducible **future installation contract**, not instructions to install nonexistent drivers. No INF can register a printer on macOS: Windows `Class=Printer` in an INF is installation metadata for the Windows setup/spooler; the user-supplied PDD-like file is separate printer-description/configuration evidence, **not a Mac installer manifest**.

### Installation choice and rationale

| Path | What is installed / registered | When to use | Costs / limits |
| --- | --- | --- | --- |
| **Preferred: native macOS app + user-space helper** | Signed `.app` containing UI, host-neutral backend, optional local IPP print/fax service, read-only diagnostics, and a controlled setup/uninstall action. Optional `SMAppService` agent/daemon in macOS 13+ for **user-approved** background processing. | Modern macOS, after backend exists. System Settings may add a discoverable local IPP queue after client compatibility is proven. | No mandatory kernel extension or INF. Must independently verify macOS FaxOut discovery and USB interface ownership/permission; service must not accept unauthenticated remote telephone-number jobs. |
| Legacy CUPS driver bundle / `.pkg` | User-domain/system-local PPD where appropriate, CUPS filter and backend compiled for host architecture, package receipts and explicitly created queue | Historical Brother macOS environments or measured compatibility need only, after original filter/protocol verification | Legacy PPD/rendering/UI plug-ins and sandbox constraints. Installing a PPD **alone** does not create a functional fax service. Apple's older instructions place third-party descriptions under `/Library/Printers/PPDs/Contents/Resources/`, **not `/System/Library/Printers`**. |
| DriverKit / USBDriverKit system extension | Signed app containing `.dext` under `Contents/Library/SystemExtensions`, `Info.plist` `IOKitPersonalities` device/interface match criteria, entitlements and System Extensions activation request | **Fallback** only if research establishes that supported user-space USB integration cannot safely access the needed interface | Entitlement approvals, per-OS signing/activation requirements, conflict risk with the printer and mass-storage interfaces. Do not attach to entire `04f9:01fd` device or seize `07/01/02` and `08/06/50` interfaces merely because the MFC includes a vendor-specific interface. |

**Info.plist vs INF vs PDD/PPD:** macOS `Info.plist` is used to describe an **app bundle**, and a DriverKit extension's `Info.plist` additionally defines optional matching personalities. A *PPD* describes print capabilities for a legacy CUPS driver. Neither the user's original Windows INF nor the PDD-like lines (`Resolution`, `InstalledMemory`, `PrinterCaps`) are a substitute for an app, service, validated device protocol, or installer.

### Planned app layout (not created)

```text
/Applications/WHP MFC-290C.app/
  Contents/
    Info.plist                # app identity, minimum OS, bundle executable
    MacOS/                    # signed universal or architecture-specific binaries
      WHP MFC-290C
    Library/                  # only if required by selected supported APIs
      LaunchServices/         # optional, app-specific support
      LaunchAgents/           # optional registered through SMAppService
      SystemExtensions/       # no .dext unless USBDriverKit fallback proven
    Resources/                # versioned configuration, not device commands
```

The actual `SMAppService` app-bundle layout must follow current Apple templates for agent/daemon/login-item type; the lines above are **illustrative**, not a finished file manifest. Use a bundle ID owned by the project only after checking namespace availability. Do not install anything into Apple's sealed `/System` volume or overwrite original Brother files.

### Safe deployment sequence — each stage has a fail-closed gate

1. **Today: read-only inventory** on a target Mac: `uname -m`, `sw_vers -productVersion`, `pkgutil --pkgs | grep -i brother`, `lpstat -v`, `lpstat -p`, and the existing `./build.sh probe-brother-mfc290c-macos`. These determine OS/architecture, existing queues/packages, and OS-visible USB identity; they **do not** show USB fax opcodes. Preserve existing queue names, ownership and negative observations.
2. **Offline simulated install:** construct a **mock-only** signed/unsigned development app bundle with a local, non-dialing status/job service and no USB OUT commands. Verify app metadata, architectures (`file`/`lipo -info`), deterministic build/fixtures, no login item installed by default, and a clean uninstall in an isolated test account. Do not register a fax queue until supported destination/job/status behavior is testable.
3. **Local IPP discovery contract:** bind to `127.0.0.1` by default, use authentication, no live fax submit capability; test real target-macOS client discovery and recipient handling. IPP FaxOut standard compliance **alone** does not guarantee Apple's Print dialog will expose the service as a working fax destination. If a local print queue is used, register it only through a deliberate user-controlled setup step after tests.
4. **USB integration gate:** only after authenticated MFC-290C fax job/endpoint/status protocol and safe interface ownership, connect the mock job service to the verified Brother send-only backend. Record errors distinctly: queue accepted, device accepted, dialing, negotiated, final remote receipt unknown/delivered/failed. Keep Mac app, CUPS adapter and any DriverKit extension separate from shared model protocol.
5. **Distribution:** for end-user deployment outside the Mac App Store, sign executable code with a suitable **Developer ID Application** identity, package as an app-in-DMG/ZIP or an optional `.pkg` signed with **Developer ID Installer** when an installer is justified; submit to Apple's **notarization** service and test with Gatekeeper enabled. If a package is needed, build from a checked **staged root** with `pkgbuild`/`productbuild` and audit exact paths, modes and scripts. Do not assume a locally compiled unsigned development binary can be distributed or loaded as a DriverKit system extension. macOS 13+ helpers should use `SMAppService` when appropriate and remain user-controlled; older macOS versions require a separately validated support strategy.
6. **Uninstall / repair:** suspend jobs, stop/deregister only the WHP-owned service, remove only the WHP-created queue after explicit choice, preserve Brother's original queues and system drivers, remove project-owned app/data and receipts as applicable, retain user documents unless separately requested, and verify no orphan startup tasks. A reinstall must not duplicate services or change phone settings or active device bindings.

**Suggested smoke-test gates (not yet implemented):** package inventory before/after is identical in no-install mode; no root requirement for read-only diagnostics; no `lpadmin`, `installer`, `systemextensionsctl`, `launchctl bootstrap`, network listener, printer reset, USB OUT, or dial on a dry run; architecture and minimum-OS validation; successful intentional install of mock-only app in a disposable environment; uninstall returns to initial state. Future CI tests can validate signed staging manifests off-device, but **a Linux CI job cannot establish successful macOS installation, signing, entitlements or actual hardware operation**.

### Monetary and operational overhead

- Apple lists Developer Program membership at **USD $99 per year** (local currency or fee waiver may vary). Budget it **only for a distribution workflow requiring Developer ID credentials**; ordinary source review and preliminary local compilation are not evidence that membership has already been purchased or is necessary at this stage.
- A `.pkg` can reduce repeated manual installation but adds maintenance of package metadata, signing, notarization and uninstall behavior. A self-contained app can avoid unnecessary system-wide writes if it meets service and device-access needs.
- DriverKit approval/entitlement work is additional cost and potential breakage; do not undertake it without a verified interface ownership/access blocker.
- Count support cost for legacy CUPS/PPD changes, Apple Silicon/intel compatibility, macOS version gates, privacy of recipients, failure/retry charges and user time.

Primary installation references:
- Apple, [Add and manage printers](https://support.apple.com/guide/mac-help/add-and-manage-printers-mh14004/mac)
- Apple, [Installing system extensions and drivers](https://developer.apple.com/documentation/systemextensions/installing-system-extensions-and-drivers/)
- Apple, [DriverKit and USBDriverKit](https://developer.apple.com/documentation/usbdriverkit)
- Apple, [Creating a driver / `IOKitPersonalities`](https://developer.apple.com/documentation/driverkit/creating-a-driver-using-the-driverkit-sdk)
- Apple, [SMAppService](https://developer.apple.com/documentation/servicemanagement/smappservice)
- Apple, [PPD placement (historical)](https://developer.apple.com/library/archive/documentation/Printing/Conceptual/UsingPPDFiles/ppd_tasks/ppd_tasks.html)
- Apple, [Packaging Mac software](https://developer.apple.com/documentation/xcode/packaging-mac-software-for-distribution)
- Apple, [Notarizing macOS software](https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution)
- Apple, [Developer Program membership cost](https://developer.apple.com/help/account/membership/program-enrollment)
- OpenPrinting, [Printer Applications replacing legacy PPD drivers](https://openprinting.github.io/cups/drivers.html)

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
