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
