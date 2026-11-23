# Brother MFC-290C — legacy fax-modem software compatibility

**Status (2026-11-23): design and evidence only; no virtual fax modem, USB fax transfer, telephone dialing, or physical-line test has been implemented.**

This is a focused continuation of [the MFC-290C protocol ledger](brother-mfc290c.md), not a new driver or a reset of its USB discovery work.

For macOS-specific fax queue, CUPS, and modern IPP FaxOut feasibility, see the [separate macOS fax integration study](brother-mfc290c-macos-fax.md); keep the shared device-facing PC-FAX backend here as the authority for cross-platform transport constraints. Its goal is to determine whether **existing programs which require a serial Class 1/Class 2 fax modem** can submit **outbound** faxes through the Brother MFC-290C's documented PC-FAX-send function.

## Evidence and scope

| Fact | Evidence | Consequence |
| --- | --- | --- |
| MFC-290C has standalone analog Group 3 fax over **LINE**, maximum 14,400 bit/s | Brother specifications and model guide | Physical fax device exists, but it is not a host-accessible modem by itself |
| Brother documents **monochrome PC-FAX sending** | Brother MFC-290C specifications and original PC-FAX documentation | A host-side outbound fax job path is a supported product feature |
| Brother explicitly excludes MFC-290C **PC-FAX receiving** | Brother PC-FAX software guide | Receiving into PC software must remain unsupported unless separately and directly proven |
| USB ID is `04f9:01fd`; interface `00` reports `07/01/02`, interface `01` reports `FF/FF/FF`, interface `02` reports `08/06/50` | User-observed USB descriptions in the parent ledger | None proves CDC/ACM, AT modem, fax opcodes or that PC-FAX belongs to interface 01 |
| Existing host driver is research-stage and has **no authenticated device-side fax command bytes** | Parent ledger and repository tests | Do not invent Brother protocol frames or use direct hardware I/O to test guesses |
| HylaFAX/efax support host-facing AT-command fax modems over serial device nodes | HylaFAX and efax documentation | A virtual serial adapter is a potential **software compatibility** interface, not evidence Brother accepts AT commands |

**Important distinction:** An analog fax signal can traverse compatible VoIP telephone equipment on the LINE side. This says nothing about whether the Brother's USB side exposes its modem as a general-purpose serial or data modem. Fax interoperability over Spectrum Voice likewise does not establish USB modem commands.

## Windows PC-FAX Printer-class INF finding (2026-11-23)

**Observation (user-reported, not independently recovered from the original installer):** A PC-FAX `.inf` reportedly has `[Version]` / `Class=Printer`. The exact INF filename, model entry, package version, OS target, and `ClassGuid` were **not provided**. Preserve this as a reported installation fact pending the authentic file; do **not** claim its full contents have been inspected.

### Meaning and boundaries

- `Class=Printer` selects the **Windows Printer device setup class**, not an assertion that paper is the final output. Microsoft's defined Printer class GUID is `{4D36E979-E325-11CE-BFC1-08002BE10318}`. A physically multifunction Brother can expose a **fax-as-print-queue** destination; Microsoft explicitly describes partner fax drivers appearing as print queues.
- The INF setup class is **not USB `bInterfaceClass=07`**, not `Class=Modem`, and not evidence for USB CDC ACM, Hayes AT commands, a serial COM port, a Class 1 or Class 2 modem, PC-FAX reception, or direct access to the Brother's analog modulator.
- Brother explicitly tells MFC-290C PC-FAX users to select **Brother PC-FAX as the printer** in the application's Print dialog, independently confirming a print-queue user workflow. A Windows print job may be rendered/processed and passed to vendor components; its actual Brother USB stream remains unauthenticated.
- Microsoft's `PRINTER_ATTRIBUTE_FAX` is a **runtime print-queue attribute** supported since Windows XP, **not something proved present by `Class=Printer` in an INF**. Query it on a real installed printer before claiming the queue sets it. Never assume the same capability or APIs on Windows 95 or DOS.

### Revised priority: spooler first, optional modem facade later

```text
Windows document -> Brother PC-FAX print queue -> spooler and Brother filter/
driver -> verified outbound Brother PC-FAX USB job (still unknown)
                                              -> fax engine -> LINE
Other modem-only fax software -> optional virtual serial compatibility adapter
                              -> same verified outbound job backend, if compatible
```

The **printer-class finding raises the priority of recovering the printer/spooler pipeline** over building a fake modem. A generic application able to Print to a fax queue may need no virtual COM port. Programs which only accept `AT+FCLASS` and direct modem control still need a carefully scoped emulation layer, and low-level Class 1 behavior might not map to a queued Brother job at all.

### Non-invasive, specific next checks on the authentic INF/package

1. Preserve **INF path, checksum, vendor signature, software package/version, target Windows release**, and whether the entry belongs specifically to **Brother PC-FAX** rather than the ordinary MFC-290C paper-printing driver.
2. In `[Version]`, record `Class`, `ClassGuid`, `Signature`, `Provider`, `DriverVer`. Confirm that `ClassGuid` actually corresponds to `Printer`; do not fill absent keys by guesswork.
3. Inspect `[Manufacturer]`, decorated model sections, exact model name, hardware/compatible ID and any **null hardware ID** for a virtual printer. A null ID is a supported virtual-printer INF pattern, not a claim that Brother uses one.
4. Identify declared `Include=ntprint.inf`, `Needs`, `DriverFile`, `ConfigFile`, `DataFile`, `PrintProcessor`, `LanguageMonitor`, `CopyFiles`, `AddReg`, dependencies and port/monitor assignments **where present**; don't invent entries. Separate UI/rendering, queue management and hardware transport.
5. For a verified installed queue, record sanitized queue name, port name, driver name, print processor, datatype, and (where supported) `PRINTER_ATTRIBUTE_FAX`. Distinguish **enqueue accepted**, **delivered to device**, **fax call completed**. No recipient telephone numbers or private document contents in logs.
6. Analyze one **offline, non-transmitting** PC-FAX spool/raster output and compare it with a paper-print job. Do not infer which USB interface carries fax data from `Class=Printer`, nor submit a real dial, reset or proprietary USB transfer during discovery.

**Evidence status:** Installation-class interpretation and Brother's print-dialog workflow are documented; user-observed original INF snippet remains unverified until supplied; USB fax job content and virtual-modem compatibility remain open.

Sources:
- Microsoft [system-defined setup classes](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/system-defined-device-setup-classes-available-to-vendors)
- Microsoft [device experience for fax print queues](https://learn.microsoft.com/en-us/windows-hardware/drivers/print/device-experience-scenarios-for-document-devices)
- Microsoft [virtual printers in INF files](https://learn.microsoft.com/en-us/windows-hardware/drivers/print/virtual-printers-in-printer-inf-files)
- Microsoft [PRINTER_INFO_2 and `PRINTER_ATTRIBUTE_FAX`](https://learn.microsoft.com/en-us/windows/win32/printdocs/printer-info-2)
- Brother [MFC-290C: select Brother PC-FAX as a printer](https://support.brother.com/g/b/faqend.aspx?c=in&faqid=faq00002726_000&ftype3=2014&lang=en&prod=mfc290c_all)

## Reported PDD resolution entry (2026-11-23)

**Reported verbatim by user from a file identified as PDD** (original filename, extension, header, software package and surrounding records not yet supplied):

```text
*Resolution 200dpi: ""
```

**Interpretation if this file follows Adobe PPD syntax:** `*Resolution` is the main option keyword; `200dpi` is a choice identifier indicating nominal symmetrical 200 dpi raster/rendering resolution; the quoted empty string is an **empty feature invocation value**, meaning **no extra PostScript command text is supplied for this particular choice**. It does **not** signify zero resolution, disabled output, a missing data stream, or proof that resolution is always the default. PPD and CUPS drivers can consume resolution as metadata to drive document rendering or select a later raster filter.

**Filename guard:** The user said **PDD**, not **PPD**; do **not** silently rename the source or assert the file is a standards-conforming PPD. The leading asterisk, keyword/choice/colon and quoted value closely resemble PPD records. Inspect the actual extension and magic/header (`*PPD-Adobe:`), encoding, product metadata, and neighboring keys to establish whether it is a PPD-format file, vendor variant, or a separately defined PDD format.

**Fax-quality association is a candidate only:** Brother's Linux PC-FAX documentation discusses `Standard` as nominal 200×100 dpi and `Fine` as nominal 200×200 dpi on supported fax models, including MFC-290C in the supported-model list. Thus `200dpi` is consistent with a nominal 200×200 **rendering choice**, but does not independently authenticate which fax transmission mode, scanning resolution, physical receiver negotiation, bit depth, encoding (MH/MR/MMR), firmware command, or USB endpoint was selected. Brother's MFC-290C product spec separately confirms PC-FAX **sending**, not PC-based receiving.

### Minimal offline follow-up on the actual original file

1. Preserve exact filename **including `.pdd` vs `.ppd`**, package/OS version, checksum, surrounding records, and original text without converting formats. Scrub any recipient numbers and personal filenames.
2. Examine `*PPD-Adobe:`, `*OpenUI *Resolution`, `*DefaultResolution`, the complete `*Resolution` choices, `*CloseUI`, `*cupsFilter`/`*cupsFilter2`, `*cupsModelNumber`, and model or product markers **only where they occur**. Compare any `200x100dpi` choice with `200dpi` rather than guessing that both exist.
3. Identify the *consumer*: renderer, CUPS filter, spooler back-end, or Brother-specific job converter. `""` does not itself issue any command to the MFC-290C. Do not send it directly as a USB payload.
4. Add an **offline parser fixture** once the file format is authenticated: parse the line exactly, distinguish `200dpi` from an empty invocation, preserve defaults, reject malformed values, and confirm parsing has no dialing/USB side effects.
5. Trace a **non-transmitting** PC-FAX job's selected resolution to its produced raster metadata; do not promote a resolution label to a verified on-wire Brother fax opcode.

**Evidence boundary:** User's exact line is an observation; its PPD-style meaning is standards-supported; the file type and device-side usage remain unverified. No new fax protocol or functional driver is claimed.

References:
- [Apple PPD Concepts](https://developer.apple.com/library/archive/documentation/Printing/Conceptual/UsingPPDFiles/ppd_concepts/ppd_concepts.html)
- [CUPS PPD Extensions](https://www.cups.org/doc/spec-ppd.html)
- [CUPS PPD compiler Resolution syntax](https://www.cups.org/doc/ref-ppdcfile.html#Resolution)
- [Brother PC-FAX (CUPS) quality documentation](https://support.brother.com/g/b/faqend.aspx?c=in&faqid=faq00100716_000&lang=en&prod=fax2820_us_as)
- [Brother MFC-290C specifications](https://support.brother.com/g/b/spec.aspx?c=us&lang=en&prod=mfc290c_all)

## Reported PDD InstalledMemory entry (2026-11-23)

**User-reported exact line, from the same source they call PDD; filename and whole file not yet inspected:**

```text
*InstalledMemory Memory: "8,72,8"
```

**PPD-like syntax (grounded):** In Adobe PPD, `*InstalledMemory` is a standard keyword to represent an installed-memory *configuration*; its option name normally matches a `*VMOption` entry so the renderer can estimate free PostScript VM. Microsoft shows examples such as `*InstalledMemory 72Meg/72 MB Total RAM: ""` as distinct **choices**. Here the option token is literally `Memory` and the quoted **invocation** is literally `8,72,8`: it does **not** match those standard memory-choice examples. A PPD normally treats the quoted portion as an invocation string, not an automatically interpreted list of installed RAM values.

**Open hypotheses, not evidence:**
- A vendor-specific range format **might** represent minimum 8, maximum 72, increment 8, but **neither the units nor triplet semantics nor which host component reads it are established**.
- If the hypothetical unit were MiB and the triplet were a range, 32 MiB is an allowed numeric point. This is a **numeric consistency check only**, not proof that the file encodes the 290C's true RAM, memory expansion, spool buffer size, fax-page quota, or SRAM/DRAM arrangement.
- This might be a PDD-like vendor configuration keyword whose meanings differ from Adobe PPD conventions; preserve the exact extension and header before assigning any parser.

**Independent hardware check:** Brother's model-specific [MFC-290C specification](https://support.brother.com/g/b/spec.aspx?c=us&lang=en&prod=mfc290c_all) explicitly gives **32 MB Memory Capacity**, while it documents PC-FAX **sending**. A separate fax-memory test-chart page rating should not be converted into raw uncompressed image storage. The source line does not contradict or replace the documented 32 MB hardware rating; how that RAM is partitioned among printing, scanning, modem and firmware remains unestablished.

**Safe next checks:**

1. Obtain the original `.pdd` or `.ppd` file with path, size, package version and checksum; inspect `*PPD-Adobe:` if present, and both adjacent settings `*Resolution 200dpi: ""` and `*InstalledMemory Memory: "8,72,8"` without silently normalizing the syntax.
2. Find any `*OpenUI *InstalledMemory`, `*DefaultInstalledMemory`, `*VMOption`, `*FreeVM`, `*?InstalledMemory`, and `*OrderDependency` **when present**; absent entries remain absent.
3. Identify the component that parses/consumes the triplet (PostScript renderer, CUPS/PPD consumer, Brother filter, spooler helper); the unusual payload must **not** be copied into a USB command or assumed to be a PostScript operation.
4. Only after the format is verified, create read-only/offline parser fixtures with malformed data, no network or printer use. Keep memory configuration separate from fax encoding, on-device memory allocation, USB transfer limits and caller-confirmed transmission status.

**Finding:** PPD-standard keyword semantics are verified, exact `8,72,8` interpretation is **unverified**. No firmware command, memory size or functional fax feature is inferred from this line.

References:
- [Microsoft, autoconfigure the printer's memory for PPD](https://learn.microsoft.com/en-us/windows-hardware/drivers/print/autoconfigure-the-printer-s-memory-for-ppd)
- [Adobe PPD file format specification](https://web.mit.edu/PostScript/Adobe/Documents/5003.PPD_Spec_v4.3.pdf)
- [Brother MFC-290C official specifications](https://support.brother.com/g/b/spec.aspx?c=us&lang=en&prod=mfc290c_all)

## Reported PDD PrinterCaps entry (2026-11-23)

**Exact user-reported line, from the same file called PDD; original file, format, package/version, and surrounding lines not yet recovered:**

```text
*PrinterCaps: "33292"
```

**What is established:** The record has PPD-like `*keyword: "value"` syntax. The value is the decimal digit string `33292`. Published CUPS PPD extensions and Microsoft PPD feature documentation inspected so far **do not establish a standard `PrinterCaps` mapping or define the bit meanings for this value**. It is a *candidate* vendor-specific capability or mode attribute, not authenticated proof that it describes fax feature flags, printer memory, modem speeds, model identity, color modes, installed options, or USB protocol.

**Representation only, not semantic decoding:**

```text
decimal:      33292
hexadecimal:  0x820C
binary:       1000 0010 0000 1100
set positions (if treated as an unsigned bitmask): 15, 9, 3, 2
set masks:    0x8000 | 0x0200 | 0x0008 | 0x0004
```

These bit positions are arithmetically correct, but `PrinterCaps` may not be a bitmask at all. **Never interpret `0x820C` using Windows `PRINTER_ATTRIBUTE_*` values** just because the PC-FAX INF has `Class=Printer`. The Windows runtime `PRINTER_ATTRIBUTE_FAX` flag is a different, separately documented context; its presence or absence cannot be inferred from this arbitrary file value.

**Context with the other exact user-reported lines (same file, not proof of shared semantics):**

```text
*Resolution 200dpi: ""
*InstalledMemory Memory: "8,72,8"
*PrinterCaps: "33292"
```

The first looks like a PPD resolution choice, the second uses a standard-like keyword with an unusual triplet invocation, and the third is an as-yet unidentified capability value. No one entry may be used to decode the others without evidence of the consuming driver/filter implementation. Keep the actual original record order when the file is acquired rather than assuming these snippets were adjacent.

### Targeted next offline checks

1. Acquire the unmodified original `.pdd` or `.ppd` (as actually named), preserve hash, file header and driver release, and locate `*PrinterCaps:` in context. Do not silently rename `PDD` to `PPD`.
2. Search the **authentic driver package** and available source, symbols, imports, and config text for the literal `PrinterCaps` and numeric `33292` (decimal), `820C` (hex), or endianness-appropriate byte patterns **only when evidence suggests binary storage**. Keep false matches separate.
3. Look for the same key across *verified* sibling model files: MFC-250C and MFC-290C first; compare fax-capable MFC-490CW against faxless DCP controls from the parent ledger. A differing numeric value is only a correlation until tied to a known feature or code path.
4. If a real decoder/consumer is discovered, document numeric type, width, signedness, units or bit definitions, version applicability, and effect on *host rendering vs. fax transport*. Require independent validation before enabling any feature based on it.
5. Only then build an **offline parser fixture** that preserves the exact numeric string, validates any allowed range and reports unresolved bits without silently enabling features. No CI dialing, phone-line use, unsolicited printer traffic, or speculative USB vendor commands.

**Status:** Syntax and arithmetic recorded. No `PrinterCaps` semantics, modem compatibility, USB fax opcodes, or physical fax operation are established by this field.

References:
- [Microsoft PPD features](https://learn.microsoft.com/en-us/windows-hardware/drivers/print/ppd-features)
- [CUPS PPD extensions](https://www.cups.org/doc/spec-ppd.html)
- [Microsoft PRINTER_INFO_2 attributes (separate runtime concept)](https://learn.microsoft.com/en-us/windows/win32/printdocs/printer-info-2)
- [Brother MFC-290C PC-FAX specifications](https://support.brother.com/g/b/spec.aspx?c=us&lang=en&prod=mfc290c_all)

## Reported BRHOOK.DLL status-monitor component (2026-11-23)

**User observation:** `BRHOOK.DLL` was found in the software package the user is investigating for Brother MFC-290C PC-FAX. No binary, directory path, file version, PE exports/imports, digital signature or hash has been provided. The DLL is **not yet authenticated as MFC-290C-specific, a fax component, a USB driver, a port/language monitor, or a modem API**.

**Corroborated context:**
- Brother's model-targeted Status Monitor documentation includes MFC-290C and describes indications such as **offline, paper empty, paper jam and ink empty**. Brother separately documents checking **ink status** via the Status Monitor (installed with MFL-Pro Suite). This establishes the software feature for this model, **not which DLL implements it**.
- Public archive listings for *other Brother models* show `BRHOOK.DLL` inside `StatusMonitor/Brmfcmon/` (e.g. DCP-8040, MFC-8440) and `StatusMon/brownie/` (e.g. HL-2140). Treat these as **cross-model provenance leads only**, not byte-identical modules or installation proof for MFC-290C. Avoid downloading or executing third-party 'DLL fix' downloads.
- Windows supports printer status through spooler **language monitors** and **port monitors** (optional, separate abstractions); Microsoft documents interface names `InitializePrintMonitor2`, `OpenPortEx`, `ReadPort`, `WritePort`, `GetPrinterDataFromPort`, and `SendRecvBidiDataFromPort`. **Do not assert `BRHOOK.DLL` exports any of them** until the actual PE export table and driver package references establish it. The filename 'HOOK' may describe a different kind of software hook.

### USER32 CallNextHookEx: Windows-only hook-chain evidence (2026-11-23)

**New user-reported static-analysis observation:** `BRHOOK.DLL` references/calls `CallNextHookEx` from `USER32.DLL`. The exact PE file, import table, disassembly and caller offsets have not yet been supplied. Preserve **import** and **executed call site** as distinct evidence levels: an import establishes a dependency/reference; an observed call instruction establishes a code path; neither alone proves which hook type or product status message is processed.

**Documented Windows semantics:** `CallNextHookEx` passes hook information to the next procedure in an already existing **Windows hook chain**; it can be called before or after the local hook procedure handles the event. It does **not** itself register a hook, poll a printer, read a USB endpoint, or call a fax modem. Windows installs hooks with `SetWindowsHookExA/W` (possibly in another DLL or process). Without the registration call or callback body, **do not** label it a keyboard hook, global hook, printer monitor, fax hook, or status transport.

**Consequence for our investigation:** The `USER32` dependency strengthens a hypothesis of **Windows application/UI-side event processing** inside `BRHOOK.DLL`; it weakens the case for treating this specific function as the underlying Brother **USB communication protocol**. It does not rule out other hardware-related behavior elsewhere in this DLL or another module.

**Read-only Windows follow-up:**
1. Inspect the exact DLL's PE import table for `CallNextHookEx`, `SetWindowsHookExA/W`, `UnhookWindowsHookEx` and any callback registration or module loading. Preserve absence as an actual negative finding when checked.
2. Identify references/call sites to `CallNextHookEx` and how their hook procedure handles `nCode`, `wParam`, `lParam` and return values; this distinguishes routine chain forwarding from actual status logic.
3. Search installer files and sibling DLL imports for who registers/loads the hook, and trace **data source -> processing -> UI notification** only where verified. Cross-reference real printer/spooler and fax-status APIs rather than assuming they are implemented by this hook.
4. Keep static analysis offline; no DLL execution/injection, `SetWindowsHookEx` experiment, device seizure, or real fax calls are needed.

**macOS isolation/portability:**
- **Do not port `CallNextHookEx` or `USER32.DLL` to macOS.** Keep it confined to an optional Windows UI compatibility component, if eventually necessary.
- Implement status as a host-neutral event/status model **after authenticated status provenance**: `device_present`, `printer_ready`, `ink_or_paper_warning`, `fax_queued`, `fax_sent_to_device`, `fax_delivery_confirmed`, `unknown`. These are prospective software categories, not Brother device codes.
- On macOS, use **IOKit device-matching/removal notifications** for USB presence, **CUPS/IPP queue and job status** for the print/fax spooler, and **native application notifications/IPC** to deliver state to a UI. These are separate from **Brother-specific device readback**, which remains unknown until actual status bytes are recovered.
- Prefer a user-space observer/spooler integration when sufficient; do not introduce a kernel extension or DriverKit extension solely to replace a Windows message hook. If a native USB transport driver is later needed, evaluate macOS's supported interfaces/entitlements on that evidence, preserving the existing read-only probe boundary.
- **Guard against false success:** printing/spooler job completion, USB connected state, or Windows UI callback activity must never be mapped to `fax_delivery_confirmed` without authenticated fax status.

**Next discriminating evidence:** The function(s) in `BRHOOK.DLL` that **call** `CallNextHookEx`, plus the presence/absence and callers of `SetWindowsHookExA/W`, rather than more generic status-monitor filenames.

References:
- [Microsoft: CallNextHookEx](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-callnexthookex)
- [Microsoft: SetWindowsHookExW](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowshookexw)
- [Microsoft: About hooks](https://learn.microsoft.com/en-us/windows/win32/winmsg/about-hooks)
- [Apple: IONotificationPortCreate](https://developer.apple.com/documentation/iokit/1514480-ionotificationportcreate)
- [CUPS: Filter and backend programming](https://apple.github.io/cups/doc/api-filter.html)
- [CUPS: IPP and job-state interfaces](https://openprinting.github.io/cups/doc/cupspm.html)

### Brother export `SetBrHook` (2026-11-23)

**New direct user observation:** The user's copy of `BRHOOK.DLL` exports a function with the exact name `SetBrHook`. This strengthens the case that the module deliberately exposes a **Brother-named Windows integration API**. It **does not** yet establish the DLL's precise model/package identity, the function prototype, whether it installs a Windows hook, or any fax/USB function. Its PE file, full export listing, ordinal, forwarding information, and function code have not yet been supplied or independently inspected.

**Windows API interpretation:** An exported DLL function may be reached by a normal import or dynamically via `GetProcAddress`. Microsoft documents a common Windows-hooks design where a DLL exposes its own *installation function* to call `SetWindowsHookEx` internally. Therefore `SetBrHook` **could** be such a wrapper, particularly alongside the earlier `USER32!CallNextHookEx` import/call observation, but **the name alone is insufficient** to assert that it installs hooks; a callback itself, a helper, or even a forwarder are alternative cases to check.

**Next read-only evidence pass, prioritized:**

1. Preserve exact original DLL file path, SHA-256, package version/model association, bitness, and the export row **name, ordinal, RVA and forwarded/non-forwarded status**. Prefer `llvm-readobj --file-headers --coff-exports --coff-imports BRHOOK.DLL` or `llvm-objdump -p BRHOOK.DLL` without executing the DLL.
2. Locate the export's RVA in disassembly (or inspect any export forwarder) and obtain a **real call graph** for `SetBrHook`: arguments, calling convention, return value, globals or stored `HHOOK` handle, references to `SetWindowsHookExA/W`, `UnhookWindowsHookEx`, `CallNextHookEx`, and window-message APIs. None of these symbols is asserted present except the user-reported export and earlier `CallNextHookEx` observation.
3. Search original, authenticated companion EXE/DLL files and installation scripts for an import or `GetProcAddress("SetBrHook")`; determine **who calls it, when, and for what event**. A `SetBrHook` export in one DLL alone does not prove that the MFC-290C driver loads or uses that copy.
4. Look for complementary Brother export names such as `ResetBrHook` or `BrHookproc` **as search candidates only**; avoid claiming they exist in the user's file until verified. If present, compare addresses and cross-references to reconstruct lifecycle while preserving negatives.
5. Keep printer/ink/paper UI notifications separate from Windows spooler job states, device USB status, and **fax delivery acknowledgments**. A Windows event hook cannot be treated as a fax transport by name.

**Cross-platform decision:** Retain `SetBrHook` as a **Windows-only optional UI/event adapter** until disassembly demonstrates actual portable business logic. A native macOS MFC-290C implementation should **not export a counterfeit `SetBrHook` or reproduce `USER32` hook chains** merely to be similar. Move any proven, OS-independent state interpretation into the shared status model; deliver it through macOS IOKit/CUPS and application-native notifications as appropriate. Continue to avoid a second status engine and never infer a delivered fax from a UI notification or print-queue completion.

**Present status:** Export name **observed by user**, documented Win32 hook pattern **verified**, correspondence between the two **hypothetical**, driver-facing USB fax and readback commands **unknown**. This update changes no runtime code and requires no hardware I/O.

References:
- [Microsoft: GetProcAddress (resolve exported DLL functions)](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getprocaddress)
- [Microsoft: Using Hooks (exported DLL installation-function pattern)](https://learn.microsoft.com/en-us/windows/win32/winmsg/using-hooks)
- [Microsoft: CallNextHookEx](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-callnexthookex)

### Evidence triangulation plan (read-only, no DLL execution)

1. Preserve exact DLL path, file size, SHA-256, Microsoft PE machine type (x86/x64), timestamp, company/version resources, and Authenticode signature status where available. Do not copy untrusted DLLs to system directories or register/load them as a diagnostic.
2. Inspect the *original associated* installation INF/CAB/script/registry materials for exact literal `BRHOOK.DLL`, `Brmfcmon`, `StatusMon`, `LanguageMonitor`, `PortMonitorDll`, `AddMonitor`, service/program names and install sections. A `Class=Printer` line in the fax INF does not imply this DLL is a spooler monitor. Preserve file-to-package-to-model mappings.
3. Perform **static** PE import/export, resource and string analysis only on a trustworthy copy, without running code: `llvm-readobj --file-headers --coff-exports --coff-imports BRHOOK.DLL`, `llvm-objdump -p BRHOOK.DLL`, or equivalent host tools. Check for spooler interfaces, GDI/USER32 window hooks, notification/event APIs, USB/Win32 device interfaces, and IPC. These are *candidate categories*, not known behavior. Avoid interpreting one import as an implemented feature.
4. Build an evidence-backed dependency map: who loads BRHOOK, which process invokes it, whether it receives status via another Brother module, and whether it belongs to **printer status** versus **PC-FAX job state**. Separate ink/paper/online data from fax dialing/negotiation/delivery receipts.
5. If a confirmed status-message format emerges, add **offline** parse tests for unknown/malformed status and truthfully maintain distinct lifecycle states: `queued`, `accepted_by_driver`, `sent_to_device`, `dialing`, `negotiating`, `delivered`, `failed`, and `unknown`. These are proposed software status names, not asserted Brother status codes. **Never turn a spooler completion into a confirmed delivered fax** or enable PC-FAX receive based on monitor presence.
6. Compare the authenticated module version and behavior with Brother MFC-250C (nearest fax cohort), MFC-490CW (higher-fax features), and faxless DCP controls, preserving differences and negative findings. The same DLL filename across products does not mean identical firmware commands.

**Unresolved now:** user file's origin/contents, exported functions, dependencies, readback path, fax-status visibility, concrete Brother USB status protocol. No runnable driver/virtual modem capability is inferred.

References:
- [Brother MFC-290C status monitor/offline guide](https://support.brother.com/g/b/sp/faqend.aspx?c=us_ot&faqid=faq00003032_001&lang=en&prod=mfc290c_all)
- [Brother MFC-290C ink/status monitor](https://support.brother.com/g/b/faqend.aspx?c=eu_ot&faqid=faq00000253_018&ftype3=2043&lang=en&prod=mfc290c_all)
- [Third-party archive listing: DCP-8040 driver package contents, cross-model only](https://www.driverguide.com/driver/detail.php?driverid=1336653)
- [Third-party archive listing: HL-2140 driver package contents, cross-model only](https://www.driverguide.com/driver/detail.php?driverid=1337612)
- [Microsoft: Language monitors](https://learn.microsoft.com/en-us/windows-hardware/drivers/print/language-monitors)
- [Microsoft: Port monitors](https://learn.microsoft.com/en-us/windows-hardware/drivers/print/port-monitors)
- [Microsoft: Installing a print monitor](https://learn.microsoft.com/en-us/windows-hardware/drivers/print/installing-a-print-monitor)

## Microsoft Fax bridge feasibility (2026-11-23)

**Scope:** Investigate a future interoperability bridge from Microsoft fax software to the **existing, still-unimplemented Brother MFC-290C monochrome PC-FAX send backend**. This is design/research only; no `faxdev.h` implementation, TAPI provider, virtual modem, fax registration, driver install or hardware test has been performed. Maintain the existing `main` branch and the model-specific USB/PC-FAX verification gates.

### Two distinct Microsoft Fax generations

| Platform | Native architecture | Candidate interoperability | Boundary |
| --- | --- | --- | --- |
| Windows 95 / 98 Microsoft Fax | Windows Messaging / Microsoft Exchange MAPI fax transport with locally configured fax modem and TAPI | Legacy MAPI fax transport integration or separately proven serial fax modem facade | **Do not reuse** Windows 2000+ Fax Service Provider API (FSP) as though it existed on Win95. The original MFC-290C Brother software did not target Win95. |
| Windows 2000 / XP Windows Fax Service | System fax service, GDI/TIFF fax printer pipeline, `faxdev.h` FSP API, TAPI devices | **Prefer optional send-only FSP with a virtual fax device** attached to the same Brother PC-FAX backend | Fax Service is separate from Brother's printer queue; `Class=Printer` and selecting Brother PC-FAX do not register an FSP or modem |
| Windows Vista / 7 / later Windows Fax and Scan | Windows fax-service infrastructure with fax-capable device/server requirements | Explore same FSP/virtual-device route only after OS/version and API availability tests | Do not claim every Windows SKU/version or CPU architecture supports custom providers; do not assume a regular USB all-in-one is a fax server |
| macOS | No native Microsoft Fax Service / `faxdev.h` | Reuse **host-neutral** fax-job backend with a CUPS/native submission adapter | No reason to carry FSP, MAPI, TAPI, or Windows `BRHOOK.DLL` UI hooks into macOS |

Historical Windows 95 Microsoft Fax setup requests a *fax device modem* through its Windows Messaging profile. The Microsoft Fax Service is documented for Windows 2000 onward. Microsoft and Brother both describe **print-to-fax** workflows but they are independently owned queues; printing a document through one does **not** register its destination as a device for the other.

### Preferred Windows 2000+ architecture (FSP virtual fax device)

```text
Microsoft Fax / Windows Fax and Scan
           |
  Microsoft fax print/TIFF Class F job and fax service
           |
  WHP Windows FSP DLL (PROPOSED; NOT IMPLEMENTED)
  - register service-provider DLL
  - FaxDevInitialize + FaxDevVirtualDeviceCreation
  - FaxDevStartJob + FaxDevSend + FaxDevReportStatus
  - FaxDevAbortOperation / FaxDevEndJob / FaxDevShutdown
  - receive entry (if required) must reject unsupported PC receive
           |
  single host-neutral, send-only fax job backend (NOT IMPLEMENTED)
           |
  authenticated Brother MFC-290C USB PC-FAX submission (NOT RECOVERED)
           |
  internal 14,400-bps fax modem -> LINE -> suitable analog telephone service
```

**Important TAPI exception:** Microsoft documents that an FSP exposing **virtual** fax devices through `FaxDevVirtualDeviceCreation` **does not need a separate Telephony Service Provider (TSP)**; a physical fax device exposed via TAPI normally does. A virtual device has no corresponding TAPI line on the Windows fax server, despite our back end eventually delivering the job to a real MFC-290C. Do not assume the lack of a separate TSP removes every FSP callback, line/event, or job-state requirement. Study the actual `faxdev.h` contract and Windows-version behavior before building.

**Document transformation:** Microsoft Fax Service produces/accepts **TIFF Class F**; Microsoft documents acceptable fax TIFF properties: compression 1 or CCITT Group 4, **1728-pixel** width, X resolution at most 204 dpi, Y resolution at most 200 dpi and matching Y-resolution across pages. These constraints must be validated for the target Windows Fax Service path, not assumed to match the Brother's own raster/USB job protocol. The earlier user-reported `*Resolution 200dpi: ""` entry in the source called PDD is **consistent with** fax-friendly rendering but proves **nothing** about TIFF compatibility, encoding, Brother USB framing, or job completion. Record a clear conversion boundary.

**Required FSP exports and statuses:** Microsoft's `faxdev.h` names `FaxDevInitialize`, `FaxDevStartJob`, `FaxDevSend`, `FaxDevReportStatus`, `FaxDevAbortOperation`, `FaxDevEndJob`, `FaxDevShutdown`, `FaxDevReceive`, with optional `FaxDevVirtualDeviceCreation` for virtual device presentation (mandatory when presenting virtual devices). An outbound-only implementation must explicitly reject or disable PC-FAX receive and still satisfy required API signatures. Do not silently promise inbound faxes just because the physical device can receive them and print them.

**Transmission and reporting:** Microsoft's `FaxDevSend` contract requires actual call initiation/data transmission/termination and `FAX_SEND` receiver metadata; truthful success requires `FaxDevSend` and `FaxDevReportStatus` to reflect actual operation, not merely a successful spool submission. Assess required callbacks and virtual-device behavior from the SDK rather than fabricating TAPI call handles. Standard fax errors include no dial tone, busy, no answer, disconnected and failed negotiation, but **Brother-specific status codes are not yet known**. A spooler 'completed' event or `BRHOOK.DLL` UI hook is not evidence that remote fax receipt occurred.

### Stage gates: bounded, offline-first

1. **Source/protocol prerequisite (blocking):** inspect an authenticated Brother PC-FAX installer/filter and a sanitized, non-transmitting monochrome job; recover exact MFC-290C USB interface/endpoint, job envelope, destination settings and device-status/error readback. No made-up command bytes or hardware I/O.
2. **Rendering contract:** validate an offline Microsoft-compatible TIFF Class F sample (width, compression, resolutions, multiple page consistency) and a separately verified Brother PC-FAX job encoding; reject unsupported color and malformed pages. Treat 200dpi PDD as descriptive until linked to a real converter.
3. **OS-split prototype:** prove a Windows 2000/XP-targeted **virtual-device FSP** can enumerate and accept a fake outbound job through a mock backend in a disposable test environment, without calling a physical number. Inspect Microsoft `FaxRegisterServiceProvider` contract and ABI, but **do not register** the provider on a user's working machine during research. Investigate Vista/7/later support separately. Treat Win95 MAPI/modem integration as an independent later branch.
4. **Failure/cancellation tests:** normalize `queued`, `submitted`, `accepted_by_device`, `dialing`, `negotiating`, `delivered`, `failed`, `cancelled`, `unknown` as proposed host states. Test timeout, busy/no dial tone, device removed, malformed job, interrupted transmission, retry duplication and cancellation without a line attached.
5. **Controlled device test (later only):** with operator-approved test number, proper physical LINE connection and safeguarded isolated environment, verify the complete remote-page result on a real MFC-290C, including how actual failure status reaches the PC. A supported Spectrum Voice analog jack may carry fax signals; that does **not** expose any fax modem directly via USB.
6. **Economic gate:** avoid buying a separate fax modem merely to test the Microsoft UI, if a verified native PC-FAX send path already exists. Compare saved hardware/service costs against the extensive complexity of FSP support, separate Win95 MAPI emulation, document conversion and maintaining two operating-system ABIs. No numerical savings are claimed without prices and measured engineering time.

**Priority decision:** Recover **one native Brother PC-FAX send backend**, then validate the Microsoft **virtual FSP** in a mock environment. A virtual COM/fax-modem layer remains a distinct, optional fallback for programs that truly require low-level Class 1/2 AT commands; it is *not* mandatory for the Microsoft Fax Service bridge if FSP works. Keep the MFC-290C's **no PC-FAX receive** limitation unchanged.

Primary references:
- Microsoft, [Setting Up Microsoft Fax for the User (Windows 95)](https://techshelps.github.io/MSDN/DNWIN95/HTML/S66CE.HTM) (historical MSDN mirror)
- Microsoft, [About the Fax Service](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/fax/-mfax-about-the-fax-service)
- Microsoft, [Fax Service Provider API Requirements](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/fax/-mfax-fax-service-provider-api-requirements)
- Microsoft, [Faxdev.h header](https://learn.microsoft.com/en-us/windows/win32/api/faxdev/)
- Microsoft, [FaxDevVirtualDeviceCreation](https://learn.microsoft.com/en-us/windows/win32/api/faxdev/nf-faxdev-faxdevvirtualdevicecreation)
- Microsoft, [FaxDevSend](https://learn.microsoft.com/en-us/windows/win32/api/faxdev/nf-faxdev-faxdevsend)
- Microsoft, [About Sending a Fax (TIFF requirements)](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/fax/-mfax-about-sending-a-fax)
- Microsoft, [FaxRegisterServiceProvider](https://learn.microsoft.com/en-us/windows/win32/api/winfax/nf-winfax-faxregisterserviceproviderw)
- Brother, [MFC-290C specifications](https://support.brother.com/g/b/spec.aspx?c=us&lang=en&prod=mfc290c_all)
- Brother, [PC-FAX Receive exclusions](https://support.brother.com/g/s/id/htmldoc/mfc/mfc-5895cw/gb/html/sug/chapter6_3.html)

## Internal fax modem and line-control boundary (2026-11-23)

**Why it matters:** Brother's MFC-290C is a standalone **Group 3 fax terminal** that can also accept **monochrome PC-FAX send jobs**, *not* an enumerated general-purpose computer serial modem. The machine's telephone `LINE` interface belongs to its fax control/modem side. The computer's `USB 04f9:01fd` interface is a **different** transport; the model-specific Brother PC-FAX job envelope, modem commands in device firmware, USB interface/endpoint used for PC-FAX, and USB readback remain unknown.

### Source-backed hardware/software division

- **Brother service manual SM-FAX098 ch. 3:** system overview identifies `Host -> USB interface` and, separately, `Line -> SDAA`, `MJ PCB` and fax-control blocks. The US/CAN MJ circuit drawing includes a line connection, SDAA interface, protection components, `A5X-G-24E-908` relay, and `1SS120` diode. These are **schematic identities**, not confirmed installed chip markings or a method for controlling the relay over USB.
- **SM-FAX098 §2.3.3:** names `TX: PC-FAX Driver` for computer-based sending, while PC receive on lower models is unavailable. Do not confuse its advanced-model `RX: Class 2` with a serial Class 2 or CDC/ACM modem exposed by **MFC-290C**.
- **Brother MFC-290C specifications:** `14,400 bps` maximum, Group 3 fax, mono MH/MR/MMR and standalone color JPEG fax, **32 MB** memory, PC-FAX **send**, no PC-FAX receive. No source identifies an internal modem IC or authenticated host-accessible AT interface.
- **Generic protocol framing, not recovered device code:** ITU-T **T.30** defines call setup, handshake and training, image transfer, post-page confirmation, and disconnect. Typical 14.4-kbit/s G3 implementations may use V.17 and V.21 control channels; we **have not identified this unit's exact modem PHY/IC, T.30 firmware routines, USB opcodes, or negotiated speeds in a captured call**. Do not implement T.30 DSP/modulation in the host merely because the hardware uses G3 fax.

### Two documented panel compatibility settings

The **MFC-290C original user guide**, Telephone Line Interference (§12, p. 100), reports:

| Operator-selected setting | Documented nominal modem rate | ECM note | Driver capability |
| --- | --- | --- | --- |
| `Normal` (default) | **14,400 bps** | Do not infer actual ECM negotiation for a call | Reportable documented metadata; **not** an authenticated PC-to-USB setting command |
| `Basic(for VoIP)` | **9,600 bps** | ECM remains available **only for standalone color fax sending** under this profile | May be operator-selected at the machine; no authenticated host USB configuration control |

This is a **maximum or nominal configured line rate**, not the rate a remote fax actually negotiates. The PC-FAX sending path is monochrome only. The device's low-speed setting can improve some VoIP circumstances but also increases ideal raw data transmission time by **50%** for identical payloads (14,400/9,600 = 1.5), before handshake, compression and retries. Disabling ECM for monochrome can remove one error-recovery mechanism; it does not establish an actual failure rate.

**Implemented minimal metadata helper:** `drivers/usb/brother/mfc290c_fax.{h,c}` now declares `WHP_MFC290C_FAX_LINE_NORMAL` and `WHP_MFC290C_FAX_LINE_BASIC_VOIP`, and `whp_mfc290c_fax_line_nominal_bps()`. It returns **14400**, **9600** or **0 on invalid mode**, never reads or writes physical settings, and makes no claims that the actual unit is currently in either mode. Host-only regression tests cover both documented profiles and invalid mode input. **This is not a functioning modem driver.**

### Where delivery status actually belongs

Brother's **original user guide**, fax Reports and Troubleshooting, documents that the machine can print a **Transmission Verification Report** with `OK` or `NG`; the troubleshooting section explicitly warns that **PC-FAX `Result:NG` can mean insufficient machine memory**, separately from telephone-line interference. The machine can queue standalone fax pages in its memory and schedule fax transmission, but **neither feature establishes a host USB status query**.

The eventual host state flow must therefore distinguish:

```text
PC app document -> driver/rendering -> USB job accepted -> machine memory/job
                    (authentic Brother PC-FAX bytes NOT YET RECOVERED)
   -> local firmware fax call/negotiation/T.30 exchange
   -> firmware transmission result -> on-device report (documented)
   -> host-readable report/USB status (?) NOT YET VERIFIED
```

For Microsoft Fax FSP, CUPS/IPP FaxOut and optional virtual COM integration, **never report successful delivery** merely because the queue drained or a USB write returned. If actual device-side delivery status cannot be read, report **unknown/pending confirmation** rather than fake `FaxDevSend` or IPP delivery success. Also do not assume a standalone received fax can be exported digitally to the PC; model-specific PC-FAX receive is unsupported.

### Bounded follow-up (offline first, no unsolicited dialing)

1. Acquire exact model/version of authentic Brother **PC-FAX Windows INF, DLLs/print processor and Mac CUPS/PPD-like file/filter**; isolate a sanitized **non-dialing** outbound job's format. `Class=Printer`, `Resolution`, `InstalledMemory`, `PrinterCaps` and `BRHOOK.DLL` are **host-side leads** and do not establish the modem's USB command interface.
2. Identify which component passes the fax **destination number**, page image, transmission options and job handle; compare against ordinary print jobs to isolate fax-specific payload framing without attaching a call-capable line.
3. Preserve read-only full USB descriptors and then map **authentic** endpoint/transfer traces under an authorized, isolated capture. Do not infer a fax endpoint from vendor `FF/FF/FF` alone or mistake mass-storage `08/06/50` for a serial modem.
4. Examine documented on-device **Transmission Verification Reports** and error categories as a **non-USB** status oracle, with telephone numbers/documents redacted. The final device-to-host status path is unresolved; protocol recovery must establish it before a Microsoft FSP or macOS fax service claims confirmed delivery.
5. Keep `Normal`/`Basic(for VoIP)` **panel-owned** until authenticated USB evidence says otherwise; tests may use mock configuration without modifying the real machine. Do not run telephone calls, analog line electrical measurements, firmware changes or relay activation just to discover the protocol.

References:
- [Brother MFC-290C original User Guide (Fax reports, telephone interference, troubleshooting)](https://manualzz.com/doc/1596870/brother-mfc-290c-all-in-one-printer-user-s-guide)
- [Brother MFC-290C official specifications](https://support.brother.com/g/b/spec.aspx?c=us&lang=en&prod=mfc290c_all)
- [Brother 2008 SM-FAX098 service manual, §§2.3 and 3.1; Appendix 6](https://manualmachine.com/brother/dcp145c/18240960-service-manual/)
- [ITU-T T.30 facsimile transmission procedures](https://www.itu.int/rec/T-REC-T.30)
- [Cisco Group 3 fax / T.30 overview](https://www.cisco.com/c/en/us/td/docs/ios-xml/ios/voice/fax/configuration/15-mt/vf-15-mt-book/Fax_and_Modem_Services_over_IP_Overview.html)

## Proposed components (strictly separate responsibilities)

```text
HylaFAX / efax / other fax program
              |
     [optional virtual TTY]
     [verified AT fax subset]
              |
       compatibility adapter
          /        \
   fax job queue   truthful status/errors
          |
  Brother PC-FAX send backend
          |
  verified USB fax submission (NOT YET RECOVERED)
          |
    MFC-290C LINE -> analog phone/compatible Voice adapter
```

1. **Preferred first milestone: native outbound job backend.** Accept a validated monochrome fax job and recipient metadata, render it into the original Brother PC-FAX submission format **only after its format and USB transfer path are authenticated**. Keep queue/USB submission independent of virtual TTY state. Existing Brother Linux `BRFAX`, `brpcfax`, `filterBRFAX`, and `brFAXrc` are prior-art research targets; their existence does not authenticate the bytes for this specific model.
2. **Optional second milestone: send-only virtual fax-modem adapter.** Provide a pseudo-terminal (or an OS-specific virtual COM interface) and implement only the AT/fax commands whose semantics can be faithfully mapped onto the verified outbound backend. A program that requires continuous low-level Class 1 modem access may not work with a job-oriented PC-FAX backend. A Class 2-style send-only facade is a *candidate*, not guaranteed compatible.
3. **Physical line is separately managed.** The Brother's rear `LINE` jack is the fax telephone connection. Its `EXT.` jack is for an external phone/answering device, not a PC modem or additional telephone line. The modem's analog call runs inside Brother firmware and may use a compatible Spectrum Voice phone jack.

## Do not advertise unsupported capabilities

- **No serial/modem claim from VID/PID or interface class.** `FF/FF/FF` means vendor-specific USB class, not known modem transport. The mass-storage interface `08/06/50` is not a fax endpoint.
- **Do not claim PC-FAX receive, `RING`, adaptive answering, or incoming fax data access.** Standalone reception may print a document and does not grant a digital document stream to the host.
- **Do not pretend to provide a conventional dial-up data modem.** `ATD` or `AT+FCLASS` compatibility for fax does not imply PPP/data calls; the USB device is not a plain Hayes modem.
- **No unconditional modem success responses.** Reject unsupported commands distinctly. Do not report `CONNECT`, `OK` for a fax transmission, completion, page success or negotiated fax settings until real backend behavior/status supports the claim. A successful local enqueue is not a delivered fax.
- **Do not assume Class 1 or Class 2 support just because a sibling model's manual uses 'Class 2'.** The original 'Class 2' entry for advanced models concerns PC-FAX receiving; it does not authenticate an AT command channel in the 290C.
- **No automatic outbound calls, hardware resets, interface seizures or firmware writes** in tests, installation, probing or CI. A live dial requires explicit operator authorization, an approved recipient and a controlled test plan.

## Interface and test acceptance gates

| Stage | Required evidence/test | Current status |
| --- | --- | --- |
| A: baseline | Existing `./build test-brother-mfc290c` USB/fax-policy tests remain green | Existing repository test target; not rerun for this design note |
| B: static driver study | Inspect authorized Brother host-side PC-FAX code/config and non-transmitting job output; record model, version, source and format | **Open** |
| C: USB mapping | Independently identify exact MFC-290C PC-FAX job framing, USB interface/endpoint, transfer direction, status and failure modes, distinct from printing/scanning | **Open** |
| D: offline backend | A mock device accepts monochrome send jobs; rejects color, zero pages, unsupported receipt and unsafe dial requests; preserves exact error/queue states | **Not implemented** |
| E: virtual serial | Offline pseudo-TTY exercises modem identity, capability negotiation, fax-send submission, unsupported receive/data commands, timeout and cancellation with HylaFAX/efax fixtures | **Not implemented** |
| F: guarded hardware | Authorized loopback/known-recipient fax tests with captured call outcomes and page-completion checks; physical LINE/VoIP path tested separately | **Not attempted** |

**Recommended integration rule:** Keep the native PC-FAX job backend as the single device-facing implementation, then add a send-only TTY compatibility layer *only if* an actual target application cannot use a simpler queue/print interface. This avoids duplicating two fax engines and keeps the Brother USB protocol separate from generic AT behavior.

## Economic hypothesis

Reusing an existing Brother fax engine may avoid purchasing and maintaining a dedicated PC fax modem and, if it shares a compatible telephone service, a second subscription. Compare avoided hardware/line costs against the time required for protocol recovery, adapter maintenance, unreliable call retries, and loss of inbound PC-FAX features. These are **hypotheses**, not measured savings; do not count a saved line that the user never needed.

## References

- [Brother MFC-290C specifications](https://support.brother.com/g/b/spec.aspx?c=us&lang=en&prod=mfc290c_all)
- [Brother PC-FAX receiving exclusions](https://support.brother.com/g/s/id/htmldoc/mfc/mfc-5895cw/gb/html/sug/chapter6_3.html)
- [Brother PC-FAX Windows/Mac instructions](https://help.brother-usa.com/app/answers/detail/a_id/58762/~/send-a-pc-fax---windows-or-macintosh)
- [Brother Linux PC-FAX CUPS documentation](https://support.brother.com/g/b/faqend.aspx?c=id&faqid=faq00100716_000&lang=en&prod=mfcl3750cdw_us_eu_as)
- [HylaFAX+ faxaddmodem](https://hylafax.sourceforge.io/man/faxaddmodem.php.html)
- [HylaFAX modem setup and AT/fax-class negotiation](https://legacy.hylafax.org/content/Handbook%3AAdvanced_Server_Configuration%3AConfiguration_Parameter_Usage_During_Modem_Setup)
- [efax manual](https://www.cce.com/efax/documentation/efax.pdf)
