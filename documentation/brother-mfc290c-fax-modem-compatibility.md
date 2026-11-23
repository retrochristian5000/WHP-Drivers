# Brother MFC-290C — legacy fax-modem software compatibility

**Status (2026-11-23): design and evidence only; no virtual fax modem, USB fax transfer, telephone dialing, or physical-line test has been implemented.**

This is a focused continuation of [the MFC-290C protocol ledger](brother-mfc290c.md), not a new driver or a reset of its USB discovery work. Its goal is to determine whether **existing programs which require a serial Class 1/Class 2 fax modem** can submit **outbound** faxes through the Brother MFC-290C's documented PC-FAX-send function.

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
