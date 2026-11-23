# ITU-T T.30 — Group 3 facsimile session procedures

**Scope:** Reusable, OS-neutral protocol research for future WHP-Drivers fax-capable devices. **Status (2026-11-23): researched reference and implementation checklist, not a complete or certified T.30 implementation.** Do not use this document alone to claim an actual device, USB interface, TAPI modem or fax transport complies with the standard.

**Authoritative edition baseline:** [ITU-T T.30 (09/2005)](https://www.itu.int/rec/T-REC-T.30-200509-I/en), *Procedures for document facsimile transmission in the general switched telephone network*, plus [Amendment 1 (01/2007)](https://www.itu.int/ITU-T/recommendations/rec.aspx?lang=en&rec=9043), which extends a silence period during fallback from V.34 to conventional Group 3 fax. The [ITU recommendation index](https://www.itu.int/rec/T-REC-T.30/en) lists both as **in force**; older T.30 revisions are superseded. Preserve the actual revision, annex, negotiated features and historical date in driver evidence: a 1990s device cannot be assumed to implement later optional features.

**Source discipline:** This is an explanatory engineering summary. The linked ITU text is the normative authority; this document does not reproduce the standard or replace its state diagrams, timers, frame formats, tables and conformance rules. Symbols below are mnemonics, **not** numeric control-frame encodings, guessed bit positions, literal USB commands or modem AT strings.

## 1. Responsibility and layer boundaries

T.30 defines **session procedures between Group 3 fax terminals**, including call establishment, optional identifications, capability exchange, modem training, page-transfer coordination, post-page confirmation, fallback and release. A Group 3 fax terminal generally implements **T.30 together with** other standards.

| Responsibility | Standard or system | Engineering boundary |
| --- | --- | --- |
| Session control, phase transitions, capability negotiation, result messages, timing | **ITU-T T.30** | Procedure/state-machine rules between fax endpoints; not a USB or operating-system installer format |
| Group 3 page formats, raster resolutions, compression options | **ITU-T T.4**; Group 4 **T.6** where applicable | How page data is represented; compression support must be negotiated/verified |
| Fax control-channel modulation | **ITU-T V.21** (historically common 300 bit/s control channel) | Electrical/modem physical layer; not the host printer protocol |
| Image-channel modulation | **ITU-T V.27ter, V.29, V.17**, or **V.34** for capable devices | Signalling rates negotiated by the terminals; a device's maximum rating is not proof of the line rate |
| Fax over IP interworking | **ITU-T T.38** | Transport of fax indicators/data over IP. T.30 procedure context still matters; T.38 is distinct from ordinary voice RTP/G.711 passthrough |
| Host fax programming interfaces | Microsoft Fax Service/FSP, TAPI, serial fax classes, CUPS/IPP FaxOut, proprietary USB PC-FAX | Application/driver job submission. None is synonymous with the T.30 signalling format |
| TIFF Class F documents | TIFF/file interchange conventions | Stored fax documents or fax-service job inputs; not T.30 call control messages |

**Do not conflate:** T.30 **does not** mandate that a USB multifunction printer enumerate as a CDC/ACM serial modem, export Hayes AT commands, implement a particular Windows INF, or expose its internal fax session via host-accessible APIs. A machine can execute T.30 entirely in firmware while a computer submits rendered pages using its proprietary printer/fax protocol.

## 2. Five phases and the C1/C2 distinction

| Phase | Session purpose | Typical observables | Host integration implication |
| --- | --- | --- | --- |
| **A — Call establishment** | Establish a connection between calling and called terminals | Ring/dial, CNG calling tone, CED called-station indication, answer/connection | No delivery claim; a call attempt and a USB job acceptance are different |
| **B — Pre-message procedure** | Identify capabilities, select agreed parameters, train the data channel | DIS, DCS, optional identification, TCF/CFR or training failure | Record negotiated configuration only if the endpoint exports evidence; unsupported options require rejection/fallback |
| **C — In-message procedure and message transmission** | Carry fax document information over the selected channel; T.30 distinguishes **C1** in-message procedures and **C2** message transmission | Image-bearing data, optional ECM flow | Page rendering and transport must not be confused with successful reception |
| **D — Post-message procedure** | Decide whether the page/document was accepted and whether additional pages or parameter changes are needed | MPS/EOM/EOP, MCF or other relevant responses | Track page-level confirmation; handle failure, retraining, continued pages, retries and partial completion |
| **E — Call release** | Terminate the fax communication session | DCN / line release | Report final call outcome separately from local queue state |

The table is an **overview**, not a complete legal T.30 transition table. Some phases can repeat, and actual exchanges vary with ECM, selected modems, optional facilities, and transfer failures. Never hardcode a simplistic *always A→B→C→D→E exactly once* progression.

### Illustrative ordinary non-ECM send sequence

```text
Caller (sender)                      Called terminal (receiver)
        |        Dial / answer / tones          |
  A     |<------------------------------------->|
        |                         DIS ----------|  receiver capabilities
  B     |---- DCS ----------------------------->|  selected capabilities
        |---- TCF / training ------------------>|  where applicable
        |<---------------------------- CFR -----|  training confirmed
  C     |---- compressed fax page/image ------>|
  D     |---- EOP ----------------------------->|  final page indicated
        |<---------------------------- MCF -----|  page/message confirmation
  E     |---- DCN ----------------------------->|  disconnection
```

**Illustrative only:** omit optional CSI/TSI, ECM and alternate responses for readability. A real capture may have retries, FTT, response delays, repeated pages, alternate modem rates or different confirmation sequences. Confirmation of transfer by the remote terminal is **not** proof a human read the document, nor proof that the local host API received that confirmation.

## 3. Common T.30 signals and evidence boundaries

| Abbreviation | Meaning / use | Avoid overinterpreting |
| --- | --- | --- |
| **CNG** | Calling tone used in automatic fax calling | Identifying a tone alone does not prove a successful call |
| **CED** | Called-terminal identification tone / answer indication | Not a fax-delivery acknowledgment |
| **DIS** | Digital Identification Signal: receiver capabilities | Capabilities reported by a terminal are not evidence that a given document used each feature |
| **DCS** | Digital Command Signal: selected transmission parameters | Select only valid combinations; do not assume an arbitrary bit layout |
| **TCF** | Training Check Function, in conventional non-V.34 procedures | Not present in every high-speed/negotiated profile |
| **CFR** | Confirmation To Receive after successful training | Establishes readiness for image phase, **not** document delivery |
| **FTT** | Failure To Train | Recovery may include retraining or fallback |
| **MPS** | MultiPage Signal: more pages, ordinarily with unchanged parameters | Page workflow continues, not final success |
| **EOM** | End Of Message: indicates procedure transition, potentially to renegotiation | Different from completing the entire fax procedure |
| **EOP** | End Of Procedure: final page/document signal | Sender's statement that sending is complete; remote confirmation still matters |
| **MCF** | Message Confirmation from the receiving terminal | Supports a page/message acceptance claim within its T.30 context; not a human-read receipt |
| **DCN** | Disconnect | Completion/disconnection alone does not identify success versus failure |
| **NSF, CSI, TSI** | Non-standard facilities and optional called/transmitting subscriber identifiers | Sensitive identities/vendor extensions are not universal requirements |
| **PPR** | Partial Page Request in ECM procedures | Handle ECM sequence/version-specific conditions, not a simple unconditional retry |

Message meaning depends on the current **phase, direction, prior state and negotiated ECM mode**. Validate a real decoder using the normative frame layout and finite-state procedures. Do not use this list to synthesize control bytes.

## 4. Error correction, training, retry and fallback

**Error Correction Mode (ECM)** is a negotiated, optional T.30 facility. It divides page data into framed units and permits selective retransmission when reception detects errors. A device's ability to send standard Group 3 fax does not prove that it always uses ECM; configuration, negotiation and firmware restrictions matter.

- Distinguish **modem training errors** from **corrupt image frames**, **busy/no dial tone/no answer**, **receiver incompatible**, **user cancellation**, **local queue failure** and **on-device memory/resource failure**.
- Implement timers, retries, post-page responses and optional messages only from the **actual applicable T.30 edition**, not guessed constants or an all-purpose retry loop.
- Rate fallback changes the raw channel rate and can affect line reliability; it must not silently change the document's identity, destination or job-level success.
- Do not retry an outbound fax automatically after the outcome is **unknown**: an earlier transmission may have succeeded, risking duplicate delivery and additional call costs.

**Nominal-rate illustration only:** for an identical fixed number of transmitted bits, 9,600 bit/s takes **1.5×** as long as 14,400 bit/s (a 50% increase in the data interval). Real fax cost/time includes handshake, page compression, retransmissions, silence/tones and telephone-service billing; the calculation does **not** predict an actual call duration or outcome.

## 5. OS-neutral state and event recording (proposed)

Keep state for the **host job** separate from T.30 session evidence:

```text
host job:
  created -> rendered -> queued -> submitted -> device-accepted
                                           -> failed / cancelled / unknown
device fax session (ONLY IF directly observed or authenticated):
  calling -> pre-message-negotiation -> transmitting-page
     -> awaiting-page-confirmation -> next-page OR disconnect
     -> confirmed-success / confirmed-failure / outcome-unknown
```

For future implementations, preserve **source, timestamp/order, job ID, page index, negotiated modem settings (if known), ECM mode (if known), T.30 phase (if visible), status provenance and confidence**. A suitable immutable record distinguishes:

- `host_queue`: e.g., print service accepted a job; **not evidence of remote receipt**.
- `device_command_ack`: machine accepted a proprietary USB job; **not evidence of dialing**.
- `protocol_event`: a genuinely decoded T.30 event from validated modem signalling; source and edition recorded.
- `device_report`: authenticated device transmission result (e.g. a physical verification report). Map its result codes independently.
- `inferred`: hypothesis used for investigation; **must not set final delivery success**.

No host adapter should convert mere CUPS/IPP `completed`, Windows spooler completion, `FaxDevSend` return, `SetBrHook` UI event, or a USB write into remote delivery proof without verified device or T.30 confirmation. Job-level success and page confirmation are separate even when a machine itself executes T.30.

## 6. Cross-device reuse, especially Brother MFC-290C

**Shared across driver families:** phase names and generic signal vocabulary, provenance-aware result mapping, safe job lifecycle, protocol boundary diagrams, offline synthetic fixtures, consent and privacy requirements. Place future decoder code in a *shared* module **only if** an actual device/host interface supplies T.30 frames or authenticated fax-session events; otherwise a pure state/status abstraction suffices.

**MFC-290C application (device-specific details stay in its own documentation):**
- Standalone Group 3 fax modem supports a **documented maximum 14,400 bit/s**; the user's model guide also documents a panel **Basic(for VoIP)** **9,600 bit/s** compatibility setting, with ECM limitations.
- Original Brother PC-FAX is **monochrome send only**; standalone color fax and receive **do not** establish PC-FAX receive or a computer-exposed serial modem.
- Brother has separate **USB 04f9:01fd** and internal **LINE/SDAA/modem** paths. Actual host-submitted PC-FAX job format, destination encoding, chosen endpoint and readback are not authenticated; the existing host-neutral `whp_mfc290c_fax_line_nominal_bps()` reports **documented nominal profile metadata**, not current line state or USB commands.
- Internal T.30 is presumed from Group 3 operation, **not traced from firmware**. Do not create host-side T.30 signaling/dial instructions for the MFC-290C unless real protocol evidence demonstrates the terminal exposes such control to the host.
- Device transmission verification reports may be useful as a *separate observational oracle*, but must not be mistaken for a USB status command.

Related model-specific studies: [MFC-290C evidence](../brother-mfc290c.md), [PC-FAX/Windows modem compatibility](../brother-mfc290c-fax-modem-compatibility.md), [macOS fax architecture](../brother-mfc290c-macos-fax.md).

## 7. Offline validation and future acceptance criteria

| Fixture / test | Expected check | Physical phone or printer I/O? |
| --- | --- | --- |
| Typical non-ECM call from a **synthetic trace** | Logical order DIS → DCS → training/CFR → image → EOP/MCF → DCN; preserve phases and provenance | **No** |
| Multipage document | MPS after intermediate pages and appropriate continuation; EOP only for final procedure | **No** |
| Training failure | FTT does not falsely declare receiver ready or job delivered | **No** |
| Invalid/unsupported capabilities | Reject unsupported negotiated modes, unexpected signals and invalid transitions | **No** |
| Optional ECM | Separate ECM and non-ECM logic; partial-page request does not become page completion | **No** |
| Error / timeout / loss of event | Fail closed to `unknown` or documented failure, no automatic real redial | **No** |
| Duplicated/out-of-order messages | Do not overwrite stronger authenticated result with lower-quality inference | **No** |
| Host/device state separation | A successful mock USB submit or CUPS job completion **never** sets remote delivery | **No** |
| Page or recipient privacy | Redact phone numbers and document contents in logs/fixtures | **No** |
| Version and vendor differences | A field or optional feature from one generation/model is **not** accepted as universal | **No** |

**Hard gates before actual protocol or driver claims:** (1) obtain the normative ITU sections for exact framing/timing, (2) identify whether the device exposes actual control frames versus abstract statuses, (3) capture ethically authorized, isolated test traffic, (4) validate sender/receiver roles, compression and ECM negotiation, and (5) separately verify remote fax results and any host readback. No unsolicited outbound calls, device resets, PSTN probing or USB transmission belongs in ordinary unit tests.

## 8. Economic and maintenance considerations

- Shared procedural vocabulary reduces duplicated Windows/macOS/Linux logic and prevents rewriting signal definitions for each model.
- Use an internal-modem submission backend when verified, rather than recreating V.21/V.17 DSP on the host without need.
- Include failed retries, time on the telephone connection, page compression, IP/VoIP loss, recipient privacy, old-driver signing/packaging and ongoing support in cost comparisons.
- **Do not count savings from a dedicated external fax modem until a real device/backend demonstrably delivers a document with truthful status.**

## Primary and complementary sources

1. **Normative:** [ITU-T T.30 index and in-force components](https://www.itu.int/rec/T-REC-T.30/en); [T.30 (09/2005)](https://www.itu.int/rec/T-REC-T.30-200509-I/en); [T.30 Amendment 1 (01/2007)](https://www.itu.int/ITU-T/recommendations/rec.aspx?lang=en&rec=9043).
2. **Image formats:** [ITU-T T.4](https://www.itu.int/rec/T-REC-T.4/en); [ITU-T T.6](https://www.itu.int/rec/T-REC-T.6/en).
3. **FoIP boundary:** [ITU-T T.38](https://www.itu.int/rec/T-REC-T.38/en).
4. **Related signalling overview:** [IETF RFC 4734, Definition of Events for Modem, Fax, and Text Telephony Signals](https://www.rfc-editor.org/rfc/rfc4734.html). It enumerates many T.30-related control events for RTP and helps with terminology, but is **not** the normative T.30 procedure specification.
5. **Model-specific:** [Brother MFC-290C official specifications](https://support.brother.com/g/b/spec.aspx?c=us&lang=en&prod=mfc290c_all) and its original manuals, cited in [our existing model ledger](../brother-mfc290c.md).

**Version policy:** Recheck the [ITU T.30 index](https://www.itu.int/rec/T-REC-T.30/en) before changing standards-based code; amend this document with the edition and approval date if the ITU lists a newer in-force component. Preserve historical requirements for older equipment.
