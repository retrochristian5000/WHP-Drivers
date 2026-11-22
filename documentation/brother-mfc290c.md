# Brother MFC-290C — USB multifunction driver evidence and boundaries

**Status:** portable USB identity/model-label decoders, synthetic host tests, and read-only macOS IORegistry/CUPS observers. **Not an installable driver.** Printing, scanning, PC-FAX, and media transport remain unimplemented.

**Evidence levels:** **Documented** = explicitly supported by Brother or a published standard; **observed** = reported by the user from a physical MFC-290C on macOS (not yet a saved raw descriptor); **implemented** = actual inspectable code and tests; **candidate** = a hypothesis requiring validation. Do not convert an observed class into proof of a working driver.

**Current host-reported USB interfaces:**

| Interface | Class / subclass / protocol | What is established |
| --- | --- | --- |
| `0x00` | `07/01/02` | Bidirectional USB printer class, not verified print stream |
| `0x01` | `FF/FF/FF`; three declared endpoints | Brother vendor class, not proven to be the scanner |
| `0x02` | Unknown | Class, role, endpoints and any media transport unverified |

These reported observations do not supply the installed firmware version, raw endpoint descriptors, or command bytes.

## Identified hardware

- Model: Brother MFC-290C, color inkjet printer, flatbed/ADF scanner and standalone analog fax.
- Computer connection: USB 2.0 **Full-Speed** (maximum signaling rate 12 Mbit/s), not 480 Mbit/s High-Speed. USB A-to-B cable; Brother recommends at most 2 m.
- Reported USB vendor/product pair: `04f9:01fd`. Cross-checked against independent scanner identification and Linux user reports. Confirm the actual attached unit's device descriptor before binding.
- Brother documents printing, scanning, card access, and **sending** PC-FAX. Standalone incoming fax is supported, but **PC-FAX receive is not available** on MFC-290C. A telephone line connects to the machine's LINE jack, not to a PC serial/modem port.
- Brother lists legacy Windows and Mac software and maintains Linux downloads. Existing third-party scanner software also supports this model. The aim is portable *new* support where feasible, not an unsupported claim that no drivers exist.

## FCC Part 15, Part 68 and ACTA evidence (2026-11-21)

**Scope:** US regulatory evidence constrains the MFC-290C's electrical
and analog telephone interfaces, not the computer-facing USB protocol.
These are three distinct identifiers: a Part 15 conformity statement,
a Part 68/ACTA telephone-equipment product label and the USB VID/PID.

| Evidence | Confirmed statement | Driver boundary |
| --- | --- | --- |
| Brother US User's Guide, Safety and Legal, pp. 89-91 | MFC-290C is declared compliant with FCC Part 15 as a Class B digital device | Emissions compliance does not establish an FCC radio-transmitter ID, WLAN hardware or USB interface behavior |
| Same guide, p. 89 | Part 68/ACTA telephone-terminal requirements; standard USOC RJ11C line connection | Applies to the analog LINE / telephone-network path, not a PC serial modem or USB CDC/ACM interface |
| Same guide, p. 89 | Rear equipment label carries an ACTA-formatted US product identifier, shown in the guide as `US:AAAEQ##TXXXX`; the `##` digits encode REN in tenths | This is a **format example**, not the machine's recovered exact identifier or REN. Do not synthesize either |
| Historical Canadian record `1112C-8CAG13`, approved 2008-04-18 | Legacy Industry Canada listing names both MFC-250C and MFC-290C | Canadian approval ID is **not** an FCC ID; family listing does not prove identical boards or firmware |
| FCC 2003 Public Notice DA 03-1061; ACTA filing database | Part 68-approved telephone equipment is listed with ACTA; the approval may use a TCB or supplier declaration | ACTA is the appropriate next US registration search; do not misinterpret the FCC's radio-device filing database as the sole authority for this fax machine |

**Verification status:** Brother's user guide establishes the existence
and expected *format* of the rear telephone-equipment label. An
authenticated exact MFC-290C US product-identifier/REN value has **not**
been recovered or verified against a physical label. No MFC-290C-specific
public FCC radio equipment authorization, internal photo exhibit, USB
firmware image, main-ASIC pinout or vendor command list was established
from these regulatory documents. This negative finding is a search
boundary, **not** proof that those materials can never exist.

**Next safe evidence pass:**

1. Inspect an authentic MFC-290C US rear rating label for the
   `US:` Part 68/ACTA identifier and REN. Keep serial numbers,
   owner information and other per-unit identifiers out of public logs.
2. Search ACTA's [Filing Search](https://part68.org/filing-search/)
   with the verified label/product ID, model and responsible party.
   Preserve the source, filing type and any applicable revision.
3. Compare any filing's **telephone-side** information with the
   service-manual MODEM/SDAA/MJ-PCB block diagram; do not translate
   analog fax compliance into USB fax command bytes.
4. Continue the independent, read-only USB descriptor inventory for
   `04f9:01fd`: all interface alternate settings, endpoints and
   packet sizes. The reported `07/01/02` interface and vendor-defined
   interface do not acquire new roles from Part 15 or Part 68.

Sources:
- Brother MFC-290C US User's Guide, *Safety and Legal*, pp. 89-91:
  https://www.bhphotovideo.com/lit_files/142624.pdf
- FCC Wireline Competition Bureau, Public Notice DA 03-1061
  (2003-04-02), Part 68 approvals and ACTA database:
  https://docs.fcc.gov/public/attachments/DA-03-1061A1.pdf
- ACTA official filing search:
  https://part68.org/filing-search/
- Legacy Canadian model record:
  https://fccid.io/1112C-8CAG13

## Rear rating-label frequency and power PCB variants (2026-11-21)

**Updated physical-label observations (2026-11-21):** The user has
reported `100-120V`, `50/60Hz` and `0.5A` from the same MFC-290C
rear rating label. Together they match the published **North American
100-120 VAC, 50/60 Hz, 0.5 A input class**. The current value is a
**nameplate input rating**, not a measured idle/printing load or proof
of internal DC rail voltages. The exact installed power-supply assembly,
its IC markings and any repair/replacement history remain unverified.
Mains frequency is not a clock frequency of the RISC ASIC, USB PHY,
scanner interface, fax modem or piezo printhead driver. These ratings
do not establish CPU ISA, USB command protocol or actual board identity.

**Independent model evidence:** Brother's June 2008 parts reference list,
PL-FAX154/8CAG13, illustrates an MFC-290CZ rating label with
`100-120V~ 50/60Hz 0.5A`. This is a **published exemplar of the Z
variant**, independently consistent with the user's own label, not
proof that their unit is a Z variant. The model-family
service manual SM-FAX098, §2.1.6, specifies US `100-120 VAC 50/60 Hz`
and Europe/Asia/Oceania `220-240 VAC 50/60 Hz`; model-specific
country/board codes still determine the actual variant. The observed
`100-120V` range **does not imply dual-voltage 100-240V** operation,
and the user's label is not evidence that the unit can be connected
to 220-240 V mains.

**Physical-label safety marking confirmed by the user (2026-11-21):**
`I.T.E. 1T22`, with a visible **UL logo** next to the marking.
The earlier `L.T.E.` transcription was explicitly corrected to
**I.T.E.** by the observer and must not remain a separate identifier
or unresolved letter reading. The user has confirmed a UL logo, but
has not separately transcribed every word or symbol surrounding it.

Brother's June 2008 MFC-290CZ reference-label illustration places
`I.T.E.` beside a **UL LISTED** mark, and service manuals for other
Brother models reproduce `I.T.E. 1T22`. In this safety-label context,
**I.T.E.** denotes *Information Technology Equipment*. `1T22`
is a **probable UL listing/control marking**, not an identified
electronic chip, PSU controller, serial number, FCC ID, USB command,
LTE modem or fax registration number. The exact meaning of the
`1T22` suffix and the MFC-290C's specific certification-file
entry remain **unverified by an authoritative UL record**.
The observed UL logo increases confidence in the safety-mark
interpretation, not in the internal PSU topology or semiconductor
identity. UL Product iQ may be used to verify the certification
entry without accessing or operating internal hardware.

**New component-selection evidence:** Brother parts reference list,
List 8 (PCB), item 2, assigns supply assemblies by region:

| Supply assembly | Listed destination(s) | Confidence / distinction |
| --- | --- | --- |
| `LT0416001` | US/CAN; includes MFC-290C/MFC-290CZ | Published regional match **strengthened by observed 100-120V, 50/60Hz, 0.5A label**; still not a physically verified installed PCB |
| `LT0485001` | European countries and several other markets | Region-specific supply, not proof of the same electrical design as US/CAN |
| `LT0660001` | ZAF/ASA/RUS | Separate assembly code |
| `LT0586001` | CHN | Separate assembly code |
| `LT0861001` | BRA | Separate assembly code |

The main board family `B53K961`, MJ PCB family `B53K967`, and
**power-supply assembly codes** are different hardware identities.
Do not use `LT0416001` as an ASIC or USB chip identifier.
Service manual SM-FAX098 Appendix 6 documents **power supply PCB
circuit diagrams**; the corresponding diagram must be matched to
the correct country/voltage version before extracting real component
references. No controller IC marking, rectifier type, switch-mode
controller part number, isolation component or output-rail voltage
has yet been verified for the actual unit.

**Hardware search order (non-destructive):**

1. Preserve the now-reported rear-label **100-120V, 50/60Hz, 0.5A**
   values without publishing any device serial number. No PCB assembly
   code has yet been observed on this machine.
2. Compare the region/model to the parts-list supply assembly and
   matching Appendix 6 power-supply schematic; record component
   designators and actual datasheet matches with provenance.
3. If safe documentation or existing unpowered board imagery is
   available, check the assembly marking. Do **not** open or probe
   mains-connected equipment to obtain it; unplugging does not
   guarantee that internal capacitors have discharged.
4. Treat primary-side rectification, EMI filtering, switching,
   regulation and isolation as **circuit-analysis candidates**
   until verified; then distinguish them from the main ASIC's
   clock tree, USB device controller and firmware command parser.
5. Never transplant a PSU across regions, assume a universal-input
   supply, or derive USB commands from the mains rating. The
   `100-120V, 50/60Hz, 0.5A` observation narrows the **power-hardware
   research path**, not the current USB parser or driver's compatibility flags.

Primary documentation:
- Brother parts reference list PL-FAX154, June 2008, front matter
  (example MFC-290CZ rating label) and List 8/PCB item 2:
  https://documents.cdn.ifixit.com/ViQd2JUqMFeULuyJ.pdf
- Brother SM-FAX098, §2.1.6 (regional AC input) and Appendix 6
  (power-supply and MJ-PCB circuit diagrams):
  https://manualmachine.com/brother/dcp145c/18240960-service-manual/
- Brother June 2008 parts-reference rating-label illustration
  (I.T.E. adjacent to UL Listed):
  https://documents.cdn.ifixit.com/ViQd2JUqMFeULuyJ.pdf
- Brother older FAX-2820/MFC-7220 service-manual label
  (explicit I.T.E. 1T22; cross-model comparison only):
  https://www.manualslib.com/manual/3821054/Brother-Fax-2910.html
- UL explanation of I.T.E. listing-mark elements and control numbers:
  https://iaeimagazine.org/2001/2001january/listing-requirements-for-ite-installed-in-computer-rooms/
- UL Product iQ certification database:
  https://productiq.ulprospector.com/en/
- Brother US MFC-290C/250C user guide, general specifications
  (North American input range):
  https://www.brother-usa.com/-/media/brother/product-catalog-media/documents/2020/08/17/10/11/um_mfc_250c_290c_es_1772.pdf

## Code contract

`drivers/usb/brother/mfc290c.{h,c}` parses a caller-owned USB device descriptor, validates its fixed 18-byte framing and checks the exact VID/PID. A match exposes raw descriptor identity fields; it **does not** imply a usable interface, endpoint, negotiated speed, driver binding, or fax connection. The feature mask describes the physical model and documented host features, not implemented operations. It deliberately excludes PC-FAX reception.

- USB controller PCI inventory (`drivers/usb/pci_hci.*`) does **not** enumerate attached USB devices; `WHPPROBE.EXE` must not report MFC-290C from an EHCI/OHCI/UHCI/xHCI controller alone.
- The user has observed a printer-class interface, a vendor-defined interface and a third interface of unknown type; no complete raw configuration/endpoint descriptor has been preserved. Do not infer a communications-class modem, scanner or storage role from the model name or interface number. Do not send vendor requests or reset the device.
- Do not confuse standalone ITU-T fax modem signaling with Hayes AT commands on a PC COM port. Any future PC-FAX submission protocol requires independently captured, authorized device evidence.
- USB transport, print stream encoding, scanner commands, PC-FAX sending, OS install packages, and real-hardware behavior remain **unimplemented**. No firmware updates, ink movement, phone calls, or USB requests occur in the identity tests or macOS observer.

## Production-era chronology and chip eligibility (2026-11-21)

**Conclusion:** MFC-290C is an **early-2008 engineering / late-2008
commercial** generation. A specialist market catalogue places the
model's *current retail listing* in **December 2008–January 2010**.
This is **not an independently certified factory-production interval**:
an official final build date has not been established.

| Dated evidence | Event and confidence | Effect on silicon selection |
| --- | --- | --- |
| 2008-03-17 | Brother European declaration of conformity, including MFC-290C; official declaration date | Model and regulated design existed by early 2008, not proof of the first production day |
| 2008-04-18 | Legacy Industry Canada certification record 1112C-8CAG13, MFC-250C / MFC-290C | Regulatory milestone before commercial rollout |
| 2008-05-26 | Brother's English HTML user manual release | Product documentation existed by May |
| 2008-06 | Brother SM-FAX098 service manual, including MFC-290C; MFC-290C hardware baseline and firmware/EEPROM sections | Original main ASIC, ROM, SDRAM, USB architecture must be evaluated as 2008-era designs |
| 2008-06-26 | Brother English quick setup guide release | Installation flow available by June |
| 2008-09-30 | US/Canada quick setup guide released | US-specific launch preparation |
| 2008-10-08 | Contemporary product introduction / Brother US HTML manual | Commercial introduction publicly reported |
| 2008-11-14 | Newegg MFC-290C first listing | Independent US retail presence |
| 2009-11-10 | ENERGY STAR model-qualification date in published equipment list | Qualification occurred after introduction; **not** a launch or factory-build date |
| 2008-12 through 2010-01 | Druckerchannel catalog's model-currency interval (`Nicht aktuell`) | Approximate period listed as current in **that regional catalogue**, not proof of continuing global manufacturing |
| Undated | Brother UK and Ireland product pages now mark model discontinued | Confirms no longer current; does not establish a discontinuation or last-factory date |

**Per-unit date-code hypothesis, private evidence:** A Brother production
date reference (revision 2011-09-12) explains that a nine-character
serial portion begins with a **month letter** followed by a **year
digit** (`J` = September; `8` = 2008). The user's previously
reported **macOS USB serial string** contains a nine-character ending
consistent with **September 2008**, which would place that unit close
to the initial launch. This is **provisional**, because we have not
compared that nine-character portion with the physical 15-character
machine label and LCD's nine-character display, nor confirmed
this exact MFC-290C unit's serial coding scheme from an authenticated
service-label image. **Do not store the actual serial, the complete
date-bearing suffix or a serial fingerprint in this public repository.**
Brother's own support FAQ (which lists MFC-290C among related models)
confirms the 15-character exterior label / nine-character LCD
distinction. The nine-character string may be visible in USB with
additional manufacturer text; no such prefix is assumed universal.

**Use as a chronological chip filter:**

- **Documented availability by 2007 / early 2008:** acceptable
  candidates subject to board markings, signal paths and specs.
- **First commercially available during late 2008:** uncertain for
  the original March–June 2008 documented hardware; require genuine
  revision or production-lot evidence before treating as plausible.
- **First released in 2009 or later:** exclude from the **initial
  2008 controller design**, absent proof of replacement, late
  production revision, or misdated component launch.
- **First released in 2011 or later:** strongly implausible for
  this model's original hardware; never infer merely because
  macOS/AArch64 currently connects to the device.
- The board family `B53K961`, including US early-production
  assembly `LT0288052` (*first three months production* in
  the parts catalogue) needs **physical board revision** and
  markings for an exact cutoff. The three-month note does not
  state the months or the factory's production start date.
- Distinguish **silicon product launch date**, **actual IC package
  date code**, **PCB assembly/manufacturing date**, **printer
  serial date**, and **retail launch/discontinuation date**.
  Dates of manuals and downstream host OS drivers do not date
  chips on the main board.

There is no verified exact CPU ISA, main ASIC IC marking, USB PHY
manufacturer, final production date, or evidence to tie the machine's
reported `FF/FF/FF` interface to one named silicon chip. We should
**not** hard-code a manufacture date or a serial match in any driver.

Sources:
- Brother manuals and release-date catalogue:
  https://support.brother.com/g/b/manualtop.aspx?c=us_ot&lang=en&prod=mfc290c_all
- Brother 2008 service manual SM-FAX098 (hardware/controller and
  production-era description):
  https://manualmachine.com/brother/dcp145c/18240960-service-manual/
- Brother 2008 European Declaration of Conformity (printed in the
  MFC-250C/290C/297C guide; 2008-03-17):
  https://download.brother.com/welcome/doc002379/cv_mfc250cz_spa_chlarg_usr_lx0948015.pdf
- Legacy Industry Canada model certification (2008-04-18):
  https://fccid.io/1112C-8CAG13
- Contemporary launch reporting (2008-10-08):
  https://news.softpedia.com/news/Brother-Presents-Three-New-Inkjet-Wireless-AIO-Printers-95139.shtml
- Newegg original catalogue listing:
  https://www.newegg.com/brother-mfc-series-mfc-290c-inkjet/p/N82E16828113327
- Specialist retail-currency interval, **not factory production**:
  https://www.druckerchannel.de/device_info.php?ID=2375&t=mfc_290c
- ENERGY STAR qualified-product list:
  https://downloads.energystar.gov/bi/qplist/image_equip_prod_list.pdf
- Brother model-specific serial-display instructions within FAQ family:
  https://support.brother.com/g/b/faqend.aspx?c=ph&faqid=faq00000300_006&lang=en&prod=mfc1810_us_eu_as
- Brother nine-character production date-code reference (unverified
  applicability to this serial's USB presentation):
  https://studylib.net/doc/18079277/how-to-determine-the-serial-number-production-date
- Original parts family and first-three-months US assembly note:
  https://www.laserexpressinc.com/manuals/Brother/brother-dcp-145c-163c-165c-167c-185c-383c-385c-387c-585cw-mfc-250c-250cz-290c-290cz-297c-297cz-490cw-790cw-990cw-parts-manual.pdf

## Inkjet-only print-engine chip filter (2026-11-21)

**Confirmed engine, not merely an ink-cartridge inference.** Brother
service manual **SM-FAX098** (June 2008), chapter 3, describes the
MFC-290C hardware family as an **inkjet, drop-on-demand piezoelectric
(PZT) head**. Section 3.2.2, pp. 3-7 and 3-11–3-13, details ink
supply tubes, dampers, a nozzle plate, piezoelectric actuators and
a **head-driver IC on the printhead's flexible circuit board**.
This driver IC is **not automatically the main-board RISC ASIC** and
is not necessarily a separate easily identifiable package on the
B53K961 main PCB. Its exact chip marking is not established.

The manual depicts a **376-nozzle, four-color** family head arrangement
(47 nozzles × two staggered lines × black/yellow/cyan/magenta) and
describes manufacturing-specific piezo response data being written
to **EEPROM on the main PCB**. The CPU uses that information for
actuator drive correction. These are **physical print-engine
requirements**, **not a USB packet format**; no safe inference about
Brother's raster encoding, nozzle firing commands or head voltage
registers follows from them.

| Candidate hardware | Filter decision for original 2008 MFC-290C |
| --- | --- |
| Piezoelectric inkjet head driver, head/carriage flex connection | **Prioritize**; manual explicitly places this driver on the head flexible board |
| Main ASIC, USB controller/transceiver, SDRAM, flash and EEPROM | **Retain**; general electronic functions are compatible with both inkjet and laser equipment. Main ASIC and USB silicon remain unnamed |
| Carriage / paper-feed / ASF motor drivers, three encoders, CIS scan interface | **Retain**; these are documented machine functions, not evidence of laser printing |
| Ink cartridge/ink-empty sensors, nozzle capping, purge/wiper and ink supply | **Retain**; documented inkjet maintenance and supply system |
| Laser-beam scanning unit and laser-diode/laser-mirror drivers | **Exclude as MFC-290C print-engine components**; incompatible with documented piezo ink deposition |
| Electrophotographic drum-charge, toner developer/transfer and fuser-specific controllers | **Exclude as MFC-290C print-engine components**; require a toner/photoconductor/fuser path absent from this inkjet design |
| Generic power converter, temperature sensor, motor IC, processor found in a laser printer | **Do not reject solely by product association**; validate part number, pinout, electrical function and 2008 availability |
| Thermal bubble-jet printhead driver | **Exclude as the known original printhead actuation technology**; the service manual specifically identifies PZT actuation |

**Two different command layers must not be merged:**

1. The Mac may transfer print-job data on the **standard bidirectional
   USB printer interface** reported by the user (`07/01/02`), or
   exchange other data on the **vendor-defined interface 1**
   (`ff/ff/ff`, three declared endpoints). Endpoint roles and
   actual command bytes remain unverified.
2. The machine's firmware/main ASIC then drives the **piezo head IC,
   carriage motors, paper feed and maintenance apparatus**. The
   internal printhead waveform is **not** an instruction that the Mac
   should construct, nor is the ASIC instruction set revealed by
   the printhead's actuator technology.

**Safety/quality guard:** EEPROM head-property data can be specific
to the installed head. Never initialize/write that EEPROM, issue
maintenance/purge/nozzle firing commands, or test with speculative
laser/inkjet commands during passive chip identification. Preserve
board revision and physical chip markings before mapping signals.

Primary manual sections and independent Brother confirmation:
- Brother SM-FAX098, §3.2.2 (printing mechanism and PZT head),
  pp. 3-7, 3-11–3-13; §3.2.3 (sensors and actuators):
  https://manualmachine.com/brother/dcp145c/18240960-service-manual/
- Brother regular cleaning-cycle explanation (includes MFC-290C):
  https://support.brother.com/g/b/faqend.aspx?c=in&faqid=faq00002016_000&lang=en&prod=dcp145c_eu_as

## Dual USB-role topology: computer connection versus front media (2026-11-21)

**Confirmed model-specific external functions.** Brother's US MFC-290C
product specifications list `PC Interface: USB` independently of the
`PictBridge (USB) Port` and PhotoCapture removable-disk features.
Brother's USB-connection FAQ locates the computer-facing receptacle
**inside the scanner housing on the right**: route its USB cable
through the internal channel and out the rear to the Mac. Calling
this simply a "rear USB port" is physically misleading.
The machine is a **USB peripheral/device** on that computer link
and communicates as vendor `04f9` / product `01fd`.

The **front USB Direct / PictBridge port** is a different USB role:
the machine is the **host** for an inserted USB flash drive or a
compatible digital camera. The original MFC-290C user guide
limits the direct port to USB flash storage or supported cameras,
including those using USB Mass Storage Class / PictBridge.
The manual specifies FAT12/FAT16/FAT32 for USB storage and
a period-specific capacity limit of 8 GB. These requirements
describe the **front port's attached peripheral**; they do
**not** imply that the Brother computer-facing interface has
`bInterfaceClass=0x08` or that the front device appears directly
in the Mac's USB topology. Likewise the model's front SD/MS/xD
card slots are locally attached media, not additional Mac USB ports.

### Physical SD/xD card-slot evidence (2026-11-21)

**New physical observation:** The user reports visible **SD** and
**xD** logos on their MFC-290C. Brother's original *User's Guide*,
chapter 11, pp. 64-66 and *Specifications*, p. 136, independently
confirms that these are actual **local PhotoCapture Center card slots**,
not decorations or USB device-interface descriptors. Brother labels
four insertion positions: front USB flash/PictBridge; SD/SDHC;
Memory Stick/PRO; and xD-Picture Card. The **front USB connector** is
separate from the **card contacts**, although both support local
PhotoCapture features.

| Medium | Official MFC-290C compatibility | Evidence boundary |
| --- | --- | --- |
| SD | 16 MB to 2 GB; miniSD/microSD via adapters | No SDXC, UHS or SDIO controller support established |
| SDHC | 4 GB to 8 GB | Listed 2008 capacity limit; not a claim about all theoretically possible SDHC cards |
| xD-Picture Card | Conventional 16-512 MB; Type M and H 256 MB-2 GB; Type M+ 1 GB / 2 GB | Different physical/protocol family from SD; presence does **not** establish a discrete xD controller chip |
| Memory Stick | 16-128 MB; Duo/Micro variants via appropriate adapters | MagicGate protected music reading not claimed |
| Memory Stick PRO | 256 MB to 8 GB | Same limitation concerning MagicGate content |
| CompactFlash | **Not supported** on MFC-290C, per Brother *Software User's Guide* | Do not copy a shared-family/retailer CompactFlash claim into MFC-290C's driver |

**Hardware hypothesis, not a board identification:** These media
types require machine-side card contacts and compatible control logic,
either integrated with the main ASIC or on a discrete reader/controller
assembly. The badges identify **supported media formats**, not how many
controllers, buses, chips, clock oscillators or PCB revisions exist.
Do not assume the card path is an internal USB mass-storage device,
a standalone SD host controller, or a memory-mapped interface.
The correct component candidate must fit the 2008-era board/assembly
and be checked against an actual part marking, service illustration
or genuine wiring record.

Brother documents **local print from card, scan to card and legacy
computer-side read/write access to inserted cards** through its
PhotoCapture software. Those are separate operation paths. The USB
transport from the Mac to the machine's media storage is still
unclassified: neither `07/01/02` nor `ff/ff/ff` nor interface `0x02`
is yet proven to be a mass-storage bridge. A visible SD/xD logo
therefore does **not** justify attaching a USB storage driver or sending
speculative vendor commands.

**Next safe research sequence:** First identify the reader/slot assembly
by model-specific Brother parts diagrams and external slot labels;
then compare authentic *unpowered*, pre-existing PCB photographs (if
available) to a reader IC datasheet. For physical testing, collect
only passive existing OS enumeration and protect user media: avoid
writes, card formatting, hot removal during activity and unknown
protocol requests. No disassembly or card insertion is required to
record these compatibility facts.

Primary sources:
- Brother MFC-290C US model specifications, PhotoCapture Center:
  https://support.brother.com/g/b/spec.aspx?c=us&lang=en&prod=mfc290c_all
- Brother MFC-290C US User's Guide, pp. 64-66, 136:
  https://www.bhphotovideo.com/lit_files/142624.pdf
- Brother *Software User's Guide*, PhotoCapture notes including
  model-specific CompactFlash exclusion:
  https://static.bhphoto.com/lit_files/35.pdf

| Port/path | Who is USB host? | What can be concluded |
| --- | --- | --- |
| Mac ⇄ Brother computer port (internal socket, cable exits rear) | **Mac** | Read-only IOKit probe describes the MFC-290C's computer-facing USB device and its interfaces |
| Brother front USB Direct ⇄ USB flash drive | **Brother** | Brother runs a local USB host-side media service; flash drive is a **peripheral to Brother**, not to macOS |
| Brother front USB Direct ⇄ supported PictBridge camera | **Brother** (supported direct-print use) | Local camera/photo workflow; not a Mac PictBridge USB port |
| Mac ⇄ Brother computer port ⇄ inserted flash/card | **Mac for outer link; Brother for front flash link** | Brother's **legacy Macintosh** Software User's Guide explicitly documents a desktop **Removable Disk** for media inserted into the machine |

**Important verified bridge:** Brother's *Software User's Guide*,
*Remote Setup & PhotoCapture Center*, **For USB cable users**,
explains that Macintosh users can access an inserted USB flash
drive or memory card through the printer's computer connection,
including reading and editing files. Brother's model specifications
also say `Removable Disk: Yes (Read/Write)`. Therefore **do not
discard host-side media access** merely because the removable drive
is physically plugged into the *front* port. However the manual
does not establish whether the computer-facing function uses a
USB Mass Storage Class interface, a vendor-specific protocol,
or another translation mechanism. The user has already reported
`07/01/02` (printer-bidirectional) and interface `0x01`
`ff/ff/ff` with three declared endpoints; **neither is
authenticated as the removable-media transport**. An additional
interface or alternate setting is possible and must be observed
rather than assumed.

The source distinguishes **local PhotoCapture** from **legacy host
access to inserted media**. These are model capability flags, not a
macOS filesystem driver, disk enumerator, USB storage-class binding
or implemented Brother transport. The exact symbols are defined in
`mfc290c.h`.

**Driver scope and non-destructive test order:**

1. Prioritize the **Mac-to-printer computer-facing connection**,
   not the front flash drive. Identify all interface numbers,
   alternate settings, class/subclass/protocol, and actual
   endpoint descriptors of `04f9:01fd`.
2. Distinguish the observed printer-class path, vendor-class
   path and any separately proven media-access function.
   Do **not** attach a USB-host driver to the front port
   from macOS or conflate USB host-side mass storage with
   USB-device-side `bInterfaceClass=08`.
3. Keep read-only device enumeration separate from media I/O.
   Brother cautions not to remove media, disconnect USB/power,
   or interrupt data transfers while the Photo Capture light
   blinks. Avoid inserting media merely for probe testing;
   if inspecting an existing mounted volume, do not modify
   files and observe proper OS ejection procedures.
4. Do not infer how many controller ICs or PHYs are on
   `B53K961`. The two role-specific ports require host
   and device functionality at the system level, but these
   functions may be integrated or distributed among chips.
   Only a main-PCB netlist/clear IC markings can distinguish.

Primary sources:
- Brother MFC-290C computer USB connector location:
  https://support.brother.com/g/b/faqend.aspx?c=us&faqid=faq00000557_002&lang=en&prod=mfc290c_all
- Brother MFC-290C US specifications (separate PC USB, PictBridge,
  read/write removable disk):
  https://support.brother.com/g/b/spec.aspx?c=us&lang=en&prod=mfc290c_all
- Brother MFC-290C user's guide, PhotoCapture Center, chapter 11:
  https://manualzz.com/doc/1596870/brother-mfc-290c-all-in-one-printer-user-s-guide
- Brother *Software User's Guide*, *Remote Setup & PhotoCapture Center*,
  **For USB cable users** (Macintosh; printed page 221):
  https://static.bhphoto.com/lit_files/35.pdf
- Brother MFC-290C service manual SM-FAX098, chapter 1 front/back
  connector identification and chapter 3 main-controller block diagram:
  https://manualmachine.com/brother/dcp145c/18240960-service-manual/

## Illuminated mode keys, control-panel PCB, and host workflow boundaries (2026-11-21)

The **four illuminated physical mode keys** are **Fax**, **Scan**,
**Copy** and **Photo Capture**, as documented in Brother's MFC-290C
User's Guide, control panel overview, and SM-FAX098
§2.1.3 *LCD/LED/Panel*. They indicate **local selectable operating
modes**, **not four USB interfaces** or four particular endpoint
addresses. Brother's *Mode Timer* setting (User's Guide, ch. 3)
returns the machine from Scan, Copy or PhotoCapture to Fax
mode after a configured delay, or remains in the last mode
when disabled. This implies a machine-managed user-interface
state, not an externally authenticated LED-control protocol.
The exact LED GPIO signals, driver ICs and firmware handlers
have **not** been identified.

The parts-reference manual (List 7, PANEL) establishes a separate
**control-panel PCB family `B53K976`** for MFC-290C:

| PCB / replaceable part | Brother code | Relevance |
| --- | --- | --- |
| Main PCB, initial control/USB candidate | `B53K961` | Main RISC ASIC and memory; USB controller/PHY relationship unverified |
| **Control-panel PCB, non-China models** | **`B53K976`, `LT0434001`** | Shared service-board family with MFC-250C/290C/297C; LED/button/LCD electronics path, *not automatically an independent microcontroller* |
| Control-panel PCB, China models | `LT0434002` | Region-specific assembly; compare physical marking before equating circuitry |
| Monochrome LCD, non-China | `LT0498001` | Display is a separately listed part, not evidence of an LCD bus exposed over USB |
| Rubber keypad sets | `LS5346001` (L), `LS5348001` (M), `LS5350001` (R) | Mechanical key contact assemblies; cannot infer a particular GPIO/key-matrix scan algorithm |

The service manual's chapter 7 replacement procedures and chapter 9
**panel PCB operational-check code 13** corroborate that the panel
is its own serviceable subsystem. **Do not enter maintenance/service
mode** solely to observe keys: its other functions can initialize
EEPROM, update firmware, reset counters or alter calibration.
Service functions are not part of a host driver interface.

### Functional triangulation (not a verified netlist)

| Physical key | Documented user function | Subsystem candidate and software boundary |
| --- | --- | --- |
| **Fax** | Select standalone fax mode | Panel PCB -> main firmware -> analog fax modem / SDAA/MJ PCB; does not create a macOS COM port or imply PC-FAX reception |
| **Scan** | Select scan mode / computer Scan-to workflow | Panel PCB -> main firmware -> CIS; Brother documents **ControlCenter's Device Button** association with the machine's Scan-to key. Actual macOS event delivery, protocol and device-service owner remain **unverified** |
| **Copy** | Select standalone copy mode | Panel PCB -> main firmware -> CIS scan + local raster/inkjet print engine; a copy need not be a USB print job |
| **Photo Capture** | Select local PhotoCapture Center mode | Panel PCB -> main firmware -> media/card interface and inkjet engine. The manual also documents access to inserted media from a computer, but **USB mass-storage or vendor-protocol binding is unverified** |

**Do not merge the machine's two USB roles:** the rear connector
enumerates as the Brother `04f9:01fd` **USB device** on macOS;
the front photo/card/USB flash/PictBridge facilities concern
**locally inserted media**, which the device may also expose as
a **legacy read/write Removable Disk** through its separate
computer-facing link. The computer link remains the relevant
driver target. A front USB socket is *not* evidence that
`interface 0x01` (`ff/ff/ff`, 3 endpoints) maps to PhotoCapture
or that the bidirectional `07/01/02` interface carries button
events. No USB endpoint addresses, transfer types or key-event
bytes are identified by the visible buttons.

The source's documented-feature function includes PhotoCapture and
tests that it is distinct from printing and scanning. This corrects
a **capability omission**, not a host-side PhotoCapture implementation.
The device may also scan to card, but no direct USB command is
inferred from that user-facing mode.

### Safe verification order

1. With the printer idle, observe only the **currently illuminated
   mode key** and LCD as the mode changes; use normal front-panel
   menus only. Do not press Start, dial, or access service mode.
2. Record how the LCD and mode LEDs respond and whether the mode
   timer returns to Fax. These are **human observations** of
   local firmware state, not USB requests.
3. On macOS, use the existing **read-only** IOKit probe before
   and after a mode change only to compare interface/service
   metadata; expect **no change** unless independently observed.
4. Search actual Brother macOS ControlCenter / scanner-button
   service evidence before implementing event notifications,
   LED setters, media-card transport, or key-triggered scans.
   Keep user serial, fax numbers and media file names private.

References:
- Brother MFC-290C user guide, control panel and mode timer:
  https://support.brother.com/g/s/id/htmldoc/mfc/cv_mfc250c/encee/html/ug/chapter1_4.html
  https://support.brother.com/g/b/faqend.aspx?c=sg&faqid=faq00002523_001&lang=en&pfs=1&prod=mfc290c_all
- Brother SM-FAX098, §2.1.3 (illuminated keys), §3.3.1
  (main controller), ch. 7 and 9 (panel assembly and service checks):
  https://manualmachine.com/brother/dcp145c/18240960-service-manual/
- Brother parts-reference list, List 7 (PANEL), control-panel
  PCB, monochrome LCD and rubber keypad codes:
  https://documents.cdn.ifixit.com/ViQd2JUqMFeULuyJ.pdf
- Brother scan-key ControlCenter/Device Button guidance,
  listing MFC-290C:
  https://help.brother-usa.com/app/answers/detail/a_id/52533/
- Brother MFC-290C user guide, PhotoCapture Center
  and computer access to inserted media:
  https://www.manualslib.com/manual/558310/brother-version-0-mfc-290c.html

## Flatbed and ADF scanner paths: shared CIS, separate mechanics (2026-11-21)

**New physical observation:** The user's MFC-290C offers scanning
from both the flatbed glass and the automatic document feeder (ADF).
Brother's model-specific specifications and the SM-FAX098 service
manual, §3.2.1 (pp. 3-5–3-7), establish that **these are two
document-motion paths through one CIS assembly**, not evidence of
two image-sensor chips or two scanner USB interfaces.

| Path | Document and CIS movement | Relevant documented hardware |
| --- | --- | --- |
| Flatbed | Original lies face-down and stationary on the glass; CIS travels along the rail and belt, then returns home | Shared color CIS module and flex cable, CIS stepper motor, drive belt, white reference film |
| ADF | Document-front sensor detects loaded paper; the CIS briefly moves for white-level compensation and then rests at ADF scan position while sheets pass over it | The **same** CIS and CIS motor; separate ADF stepper motor, pickup/separation/feed rollers, document-front and document-rear sensors |

The service manual states that inserting a document activates
automatic **ADF-path selection** at scan start. This is a behavior of
the machine's control firmware, not proof that a given host USB
command selects the ADF. The machine's color CIS alternates
**red/green/blue LEDs**; monochrome uses green illumination. This is
evidence for illumination sequencing and image-sensor electronics,
not permission to drive LEDs or motors directly from the host.

**Published MFC-290C scanner limits** (Brother US specifications):
- ADF: up to **15 sheets**; document width **148–215.9 mm**,
  length **148–355.6 mm**.
- Flatbed glass: document area **215.9 × 297 mm**.
- Listed scanner **optical resolution 1200 × 2400 dpi** and
  internal memory **32 MB** are product-level specifications.
  Per-mode maximum resolutions, image transfer buffering,
  supported pixel formats and whether the firmware streams scans
  have **not** been established by those numbers.
- The ADF is a sheet feeder with one shared imaging line:
  **automatic duplex scanning is not demonstrated**. Do not
  advertise a duplex capability from ADF presence alone.

**Component eligibility — identify function before manufacturer:**

| Component class | Evidence / reason to inspect | Still unknown |
| --- | --- | --- |
| CIS sensor PCB / analog front end and image acquisition | Documented CIS RGB illumination, lens array and pixel output | Sensor part marking, ADC location, controller integration, sampling protocol |
| CIS stepper drive | Documented belt/rail motion including calibration positioning | Motor-drive IC or main-ASIC integration, coil specifications |
| ADF stepper drive | Documented separate feeder motor in ADF models | Driver IC, control wiring, shared or separate motor IC |
| Document sensor conditioning | Document-front and document-rear sensors and their actuators | Sensor electrical interface and main-PCB pins |
| Main ASIC / SDRAM | Coordinates scan workflow and data path; memory capacity independently listed | Image processing, buffers, USB command routing and exact ISA |

**Driver scope:** Keep one scanner-device identity with **two
document-source capabilities** only when host protocol evidence can
expose and select them correctly. Plan for single-sheet flatbed and
multi-page ADF workflows, page boundaries, calibration, paper-empty,
jam and cancellation handling; these are *design requirements*, not
implemented behavior. No scan acquisition, ADF actuation, USB vendor
requests, firmware actions or register writes are added by this
documentation. Never infer that interface `0x01` or `0x02`
is the scanner solely from the observed mechanical features.
Inspect the existing Brother macOS application/driver behavior
and authorized passive USB capture before implementing commands.

**Best next evidence target:** Match the scanner/ADF assembly and
main-PCB cable headers in the *model-specific* service/parts drawings
to documented CIS/ADF motor wiring. Use an authenticated, already
available board photograph or component marking to identify
any actual IC candidate; do not guess it from a feature list,
open an energized machine or experimentally move the scanner.

Primary references:
- Brother MFC-290C US specifications (CIS, 15-sheet ADF,
  document sizes, optical resolution, 32 MB):
  https://support.brother.com/g/b/spec.aspx?c=us&lang=en&prod=mfc290c_all
- Brother SM-FAX098, chapter 3, §3.2.1, pp. 3-5–3-7
  (CIS, calibration, feeder, motors, sensors):
  https://manualmachine.com/brother/dcp145c/18240960-service-manual/
- Brother model-specific FAQ on multi-page scan workflows:
  https://support.brother.com/g/b/sp/faqend.aspx?c=us_ot&faqid=faq00002578_001&lang=en&prod=mfc290c_all

## Physical widths and candidate fit screening (2026-11-21)

**Scope: constraint-based hardware triangulation, not chip identification.**
Brother's [MFC-290C US specifications](https://support.brother.com/g/b/spec.aspx?c=us&lang=en&prod=mfc290c_all)
give **390 × 375 × 180 mm** (W × D × H) for the machine body
and **7.8 kg** mass. Brother's
[original dimension drawing](https://download.brother.com/welcome/doc002379/cv_mfc290cz_canfre_usr_lx1023002.pdf)
also shows **410 mm extended width** and **460 mm extended depth**
when supports/projecting parts are counted. Neither external
measurement is the inside clear space nor the main PCB, PSU PCB,
scanner module or media-reader assembly dimensions. Do **not**
estimate an IC package or board area from a whole-printer silhouette.

| Reference | Published limit | Physically relevant candidate class |
| --- | --- | --- |
| Body external width / depth / height | 390 / 375 / 180 mm | Only the **outer dimensional envelope**; actual mechanical cavities must be smaller and measured separately |
| Extended width and depth | 410 / 460 mm (Brother drawing) | External supports; **never** substitute for available internal rigid-board span |
| Scanner glass document width | 215.9 mm maximum | Document placement, **not** active sensor-chip package width |
| Scanner effective image width | 210 mm | Minimum required **effective CIS imaging coverage**; not proof of a single 210-mm semiconductor die |
| ADF document width | 148–215.9 mm | Feeder guides/rollers/document pathway; not chip-package fit |
| Normal print width | 8.26 in for Letter per Brother, 8.5 in borderless | Carriage travel/image geometry; no direct ASIC or head-driver IC package dimensions |

The physical subassemblies are separately identified in Brother
[SM-FAX098](https://manualmachine.com/brother/dcp145c/18240960-service-manual/)
as main PCB, MJ/PS shield and PSU PCB, and scanner/ADF mechanisms.
Published **board width, board depth, mounting holes, enclosure
clearance, component height limits, available connector orientation
and installed IC package sizes remain unmeasured** for the actual
unit. Exploded-view drawings or service illustrations without a
declared scale are not dimensional evidence for millimeter-accurate
chip fit.

**Physical-fit elimination protocol:**

1. **Locate** the right assembly and model/region: main PCB family
   `B53K961` (MFC-290C US assemblies `LT0288051` or early
   `LT0288052`), US/CAN PSU candidate `LT0416001`, MJ board
   `B53K967`, CIS and reader. Do not transfer one board's usable
   envelope to another.
2. **Measure/document** the candidate site in three axes, mounting
   footprint, obstructing chassis walls, headroom, connector/cable
   access, necessary insulation/creepage/clearance and thermal space.
   Unscaled photos cannot provide hard dimensions without a known
   physical reference and perspective correction.
3. **Read the real chip's package mechanical drawing** (including
   width/length/height and tolerances), recommended land pattern
   and pinout. Distinguish die size, plastic package, PCB pad pattern,
   heat sink and functional module dimensions.
4. **Check allowable orientation** and assembly constraints. Reject
   a candidate as *physically impossible* only when a verified
   package/module envelope cannot fit the **measured internal
   footprint/clearance** in any plausible mounting orientation.
   If a dimension, board location or package is unknown, mark
   `not-evaluated` rather than `excluded`.
5. **Corroborate** the fit with 2008 component availability, actual
   board markings, power requirements, connector signals, CIS/ADF
   functions and full-speed USB role. A chip **fitting** is necessary
   but not sufficient; it does not establish its identity or the
   USB protocol.

**Candidate outcomes:** `excluded-by-dimension` only with
both source-backed mechanical envelope and candidate data;
`dimensionally-plausible` where constraints are known to fit;
`not-evaluated` for missing dimensions. Record exact
assembly, revision, evidence provenance and which axis fails.
Never reject ordinary 2008-era IC packages using the 390-mm
printer width alone, and never infer the internal scanner controller
from the 210-mm *scan-area* width.

**Non-invasive boundary:** use original technical manuals,
manufacturer datasheets and authenticated *existing* unpowered
board photos. No disassembly, energized AC/telephone probing,
scanner carriage motion, chip swaps, card writes or USB requests
are needed for this documentation. There are **no verified
dimension-based IC exclusions yet**; the new results are constraint
definitions and a targeted measurement plan, not a found chip.

Sources:
- Brother MFC-290C specifications, general, scanner, printer:
  https://support.brother.com/g/b/spec.aspx?c=us&lang=en&prod=mfc290c_all
- Brother MFC-290CZ Canadian French User's Guide, printed p. 140,
  MFC-290C front/side dimension drawing (PDF page index 153):
  https://download.brother.com/welcome/doc002379/cv_mfc290cz_canfre_usr_lx1023002.pdf
- Brother June 2008 parts-reference PCB list (regional board codes):
  https://documents.cdn.ifixit.com/ViQd2JUqMFeULuyJ.pdf
- Brother June 2008 SM-FAX098 service manual, chs. 1 and 3,
  scanner/CIS and separated main/MJ/PS assemblies:
  https://manualmachine.com/brother/dcp145c/18240960-service-manual/

## Main PCB chips and USB command-processing candidates (2026-11-21)

**Hardware evidence, not a working driver.** Brother's June 2008
SM-FAX098 service manual (MFC-290C included) identifies a **RISC 128 MHz**
CPU for the lower-tier MFC-250C/290C models in section 2.1. Its
section 3.3.1 control-electronics diagram identifies the **Main ASIC**,
**SDRAM**, **ROM**, **MODEM**, **EEPROM**, **SDAA** (telephone-line interface)
and **motor-driver circuits** connected through the main PCB. The manual
does **not** name the CPU instruction set or give a readable IC manufacturer
marking, model number, USB controller register map, or main-ASIC pinout.
In particular, RISC does **not** imply AArch64/ARM, MIPS, or any other
specific ISA.

| Evidence | MFC-290C-specific significance | Limit |
| --- | --- | --- |
| Main ASIC + 128 MHz RISC CPU | First candidate for interpreting firmware-level commands after USB reception | The USB PHY and peripheral controller may be on-chip or separate; no die, pin or register map established |
| SDRAM | Working data buffering while machine handles host jobs | No capacity, address map, USB DMA or packet-buffer location established |
| Flash ROM / program ROM | Stores executable machine firmware (Appendix 2) | No CPU ISA, firmware format, command dispatcher or image authorization established |
| EEPROM | Stores destination/model/calibration parameters (Appendix 3) | Not a USB command register and must not be written during inventory |
| MODEM and SDAA / MJ PCB | Fax-line path separate from host computer's USB port | No reason to infer CDC ACM, COM port or AT-command USB exposure |
| DC/step motor drivers and CIS/front-end | Downstream print and scan actuators/sensors | Do not send motion or maintenance commands merely to identify silicon |

The Brother **parts reference list**, List 8 (PCB), identifies main-board
family **B53K961** for MFC-290C. It lists **LT0288051** for many countries,
**LT0288052** for US first-three-month production, and **LT0288055** for
China. The separate **MJ PCB** is classified as **B53K967**, including
**LT0405001** for MFC-290C in US/CAN/CHN/HK/ARG/CHL/ASA/OCE. These are
PCB/assembly codes, **not ASIC chip part numbers** or interchangeable
firmware versions. Related higher-tier MFC-490CW has main PCB **B53K960**;
its silicon/protocol must not be presumed identical.

**Current USB receive-chain hypothesis (not a verified circuit trace):**

    macOS host USB transaction
        -> rear MFC-290C USB device connector (USB 2.0 Full-Speed)
        -> USB device transceiver / protocol controller (unknown IC)
        -> main ASIC / firmware command handling (candidate)
        -> SDRAM/ROM and function-specific print, CIS scan or PC-FAX path

The service manual is a **functional/wiring** reference; Appendix 6
specifies detailed circuit diagrams for the **MJ and power supply PCBs**,
not a complete main-PCB USB netlist. It does not distinguish whether
the USB transceiver, endpoint state machines and RISC CPU share a chip.
A seller's photograph of an alleged `E131175` replacement board cannot
authenticate silicon markings, assembly revision, or exact component layout.

**Evidence gates before writing driver commands:**

1. Record readable **top/bottom main PCB photographs**, exact board revision
   and non-destructive chip-package markings for an authenticated MFC-290C.
   Do not probe an energized/open printer or telephone-line circuitry.
2. Identify the USB-receiving silicon and obtain its manufacturer
   datasheet or documented pinout; keep firmware ISA separate from USB
   descriptors. Do not infer an external USB chip from visual appearance.
3. Collect sanitized, read-only actual USB **device and configuration
   descriptors**, interface numbers, alternate settings, endpoint types
   and packet sizes through OS-managed inspection; no interface seizing.
4. Compare independently authorized printer versus scanner versus PC-FAX
   host-driver evidence **per function**. Do not promote protocol requests
   from related Brother models into MFC-290C commands without confirmation.

**What is now established:** a ranked USB/ASIC investigation target,
hardware block inventory and board assembly IDs. **What remains
unestablished:** exact USB controller/PHY chip, its internal/external
relationship to the main ASIC, firmware CPU ISA, vendor protocol bytes,
driver-ready printing/scanning/PC-FAX functions.

Primary references:
- Brother SM-FAX098, section 2.1 (128 MHz RISC model tier), section
  3.3.1 (control-electronics block diagram), Appendix 2/3 (flash/EEPROM),
  Appendix 5/6 (wiring and circuit diagram scope):
  https://manualmachine.com/brother/dcp145c/18240960-service-manual/
- Brother DCP/MFC Parts Reference List, List 8 (PCB), B53K961/B53K967
  model and region-specific assembly identifiers:
  https://www.laserexpressinc.com/manuals/Brother/brother-dcp-145c-163c-165c-167c-185c-383c-385c-387c-585cw-mfc-250c-250cz-290c-290cz-297c-297cz-490cw-790cw-990cw-parts-manual.pdf

## Per-unit USB serial string evidence (2026-11-21)

A physical MFC-290C has been **reported by the user** in the macOS
System Information USB view with a serial-number string. The exact
per-device value is deliberately **not recorded in this public repo**.
This is a host-observed identifier, **not a board part number, firmware
version, USB endpoint or vendor command**, and it does not identify which
chip implements USB string-descriptor responses. Confirm the value is
shown under the MFC-290C device's USB entry alongside `04f9:01fd`.

Standard USB device-descriptor byte **16**, `iSerialNumber`, is a
**string-descriptor index**: zero means no USB serial string is
advertised; nonzero means the host may request that string using USB
standard enumeration. The index itself is **not** the serial number.
The portable `whp_mfc290c_usb_identity.serial_string_index` field now
exposes this byte from **caller-provided raw device descriptors**,
without submitting any control transfer or accessing the printer.
Synthetic tests cover index 3 and index 0, plus existing rejected
descriptor/output-preservation cases. These fixtures do not assert the
physical MFC-290C's actual index.

The existing macOS **read-only** IORegistry observer now prints only
`serialRegistry=present` or `serialRegistry=unavailable` for each
matching `04f9:01fd` device. It uses the existing USB-device entry's
`USB Serial Number` property (Apple's `kUSBSerialNumberString` key).
An unavailable property is not proof of descriptor index zero. Neither
string contents nor serial-derived hashes are emitted or stored.
Do not hard-code a user's serial into a driver, CI fixture, output
filename, log or public evidence ledger. Do not use it to authorize
device updates or to infer internal RISC/USB-controller silicon.

**Safe physical cross-check:** the MFC-290C User's Guide, *Machine
Information → Checking the serial number*, directs users to press
`Menu → 6 → 1` on the device LCD, then `Stop/Exit`. Privately compare
that LCD value with the macOS System Information USB entry. A match
raises confidence they refer to the same machine; a mismatch calls
for checking the device entry, host-side cached metadata and exact
serial representation, not inventing a USB command.

Sources:
- USB 2.0 device descriptor's `iSerialNumber` at offset 16:
  https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-xusbi/5ae30d66-029d-4c7c-b43b-567c298bddab
- Apple `kUSBSerialNumberString` IORegistry property:
  https://developer.apple.com/documentation/iokit/kusbserialnumberstring
- Brother MFC-290C User's Guide, serial number LCD instructions, p. 118
  (online viewer page 126):
  https://www.manualslib.com/manual/558310/Brother-Version-0-Mfc-290c.html?page=126

## Firmware, flash ROM and service-mode boundary (2026-11-21)

Brother publishes a historical MFC-290C firmware release **G**
dated **2011-09-08**. That release listing does **not** establish
the firmware installed on the user's machine.

Installed firmware **might** affect USB enumeration, vendor commands,
print/scan behavior or PC-FAX, but no firmware-dependent variation
has yet been demonstrated on the MFC-290C. Do not introduce guessed
version-specific commands, defaults or failure diagnoses. The current
parser and macOS probe contain **no installed-firmware detection**.

The USB device-release field `bcdDevice` must be distinguished
from Brother's LCD firmware letter; a change in one is not proof
of a change in the other. Flash firmware, configuration/calibration
EEPROM and firmware switches are also distinct. No maintenance
mode, EEPROM writes or updater execution is needed for diagnosis.

See the [firmware evidence and compatibility page](brother-mfc290c-firmware.md)
for official sources, a read-only LCD check and verification criteria.

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

## FreeBSD USB driver cross-check (2026-11-21)

Scope: public FreeBSD `src` USB sources as reviewed on 2026-11-21,
not a claim of tested FreeBSD support on the physical MFC-290C.

| Source | Finding | Safe consequence for this project |
| --- | --- | --- |
| `sys/dev/usb/usbdevs` | Lists Brother vendor `0x04f9`, plus older HL-1050 and MFC-8600/9650 entries; **no MFC-290C/`0x01fd` product entry found** | Lack of a product-list entry does **not** exclude generic USB class-driver binding |
| `sys/dev/usb/serial/ulpt.c` | `ulpt_devs` matches standard USB printer interface class `0x07`, subclass `0x01`, protocols uni/bidirectional/1284; the attach logic searches an alternate interface suitable for bidirectional use | Printer transport remains a **candidate** until this model's actual interfaces, alternate settings and endpoints are known |
| `sys/dev/usb/quirk/usb_quirk.c` | No Brother name, `0x04f9` or exact `0x01fd` entry found in the reviewed static quirk table | No MFC-290C-specific quirk established; does not exclude runtime quirks or other sources |
| `sys/dev/usb/usb_parse.c` | `usb_desc_foreach` bounds descriptor iteration using a prevalidated `wTotalLength`; interface and endpoint descriptor lengths are also checked | A future offline raw-configuration decoder must reject incomplete, zero-length and overrun records and retain alternate-setting context |

**Important implementation limit:** `ulpt.c` issues USB printer-class
SOFT_RESET using both legacy `0x23` and corrected `0x21` request types,
and `ulpt(4)` documents an alternative `unlpt` no-reset device path
because some printers misbehave on reset. The existing WHP offline
SETUP classifier already distinguishes these captured requests.
FreeBSD's behavior is **not evidence that the MFC-290C accepts or needs**
those resets. Our macOS observer must never send them. FreeBSD also
contains a disabled GET_DEVICE_ID path with compatibility warnings;
that does not authenticate this model's IEEE 1284 device ID.

**Read-only macOS improvement:** the IOKit USB interface inventory now
reports `alternate=0xNN` and `declaredEndpoints=0xNN` when numeric
`bAlternateSetting` and `bNumEndpoints` properties are visible,
otherwise `unknown`. They are **IORegistry metadata hints**, not an
authenticated complete raw USB configuration dump. The observer does
not set an alternate interface, claim endpoints, open a pipe, or send
USB requests. It reports available interface metadata alongside class,
subclass and protocol; no printer/scanner/fax roles are inferred.

For **a separate authorized FreeBSD host**, `usbconfig` provides
non-mutating inspection commands after selecting the correct `ugenX.Y`:

```sh
usbconfig
usbconfig -d ugenX.Y dump_device_desc
usbconfig -d ugenX.Y dump_curr_config_desc
usbconfig -d ugenX.Y show_ifdrv
```

Here `ugenX.Y` is a placeholder to replace with the observed target.
These are examples **for FreeBSD, not macOS commands**. Sanitize serial
strings and other unit-specific data before sharing traces. Never run
`do_request`, `reset`, `set_alt`, `detach_kernel_driver`, firmware
updates or print/scan/fax transactions merely to identify hardware.

Remaining evidence needed: verified `04f9:01fd` physical configuration
descriptor with each interface number, alternate setting, class,
subclass, protocol, endpoint type/direction/address and maximum packet
size; then identify which function/driver owns each interface. Generic
FreeBSD class handling does not reveal Brother's raster/scanner/fax
commands or the main ASIC's instruction set.

References:
- FreeBSD `usbdevs`: https://cgit.freebsd.org/src/tree/sys/dev/usb/usbdevs
- FreeBSD `ulpt.c`: https://cgit.freebsd.org/src/tree/sys/dev/usb/serial/ulpt.c
- FreeBSD `usb_quirk.c`: https://cgit.freebsd.org/src/tree/sys/dev/usb/quirk/usb_quirk.c
- FreeBSD `usb_parse.c`: https://cgit.freebsd.org/src/tree/sys/dev/usb/usb_parse.c
- FreeBSD `ulpt(4)`: https://man.freebsd.org/cgi/man.cgi?query=ulpt&sektion=4
- FreeBSD `usbconfig(8)`: https://man.freebsd.org/cgi/man.cgi?query=usbconfig&sektion=8
- Apple USB interface descriptor: https://developer.apple.com/documentation/kernel/iousbinterfacedescriptor

## Shared probe core and platform-specific boundaries

The MFC-290C macOS observer shares a small source-reporting
module with the DOS `WHPPROBE.EXE`:
`tools/probe/probe_scope.{h,c}`. This **unifies report
provenance, not hardware access**. The macOS tool obtains
interface metadata through IOKit and CUPS; DOS probes
BIOS/PCI devices and cannot enumerate the MFC-290C through
a USB host-controller identity. Both commands remain
read-only; their distinct USB visibility is explicit in
the shared report preamble. Neither gains another operating
system's drivers or commands from this arrangement.

Use `./build test-probe-scope` to validate the common
contract without accessing a device. Physical-device
USB transfers, print/scan commands and DOS USB peripheral
enumeration remain unimplemented.

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

## Interpreting macOS `protocol=0xff` (2026-11-21)

A user-observed `protocol=0xff` from the **original macOS IOKit probe**
is a readable `bInterfaceProtocol` byte with numeric value 255;
it is **not** the probe's `protocol=unknown` fallback. This field
must be interpreted **with `bInterfaceClass` and `bInterfaceSubClass`**
from the *same interface*, not a device-level class or a different
alternate interface. **Updated user observation:** the original macOS
System Information/IOKit probe showed one MFC-290C interface with
`class=0xff subclass=0xff protocol=0xff`. This is a **user-reported,
host-observed triplet**, not a preserved raw configuration descriptor
or an independently reproduced hardware test. **Further observations
from the same physical MFC-290C (user report, 2026-11-21):** the vendor
interface is `number=0x01` with `declaredEndpoints=0x03`.
Another probe record printed `protocol=0x02` and
`protocolMeaning=printer-bidirectional`; our code emits that label
only when the same record has `class=0x07 subclass=0x01 protocol=0x02`.

| Reported interface record | Number | Class/subclass/protocol | Endpoints | Evidence status |
| --- | --- | --- | --- | --- |
| Vendor class | `0x01` | `ff/ff/ff` | `0x03` (3) | User-reported IORegistry fields |
| Bidirectional USB printer class | `0x00` (new report) | `07/01/02`, explicitly observed | Not yet reported | User-reported IORegistry fields |
| Third interface | `0x02` (new report) | **Unreported** | **Unreported** | Its existence as an interface number is reported; its function is unknown |

**Updated topology:** The user has now identified three **distinct
interface numbers** on the same MFC-290C computer USB device:
`0x00` is the printer-class interface, `0x01` the vendor-class
interface, and `0x02` remains unclassified. The original lines'
`bAlternateSetting`, `bConfigurationValue` and raw configuration
descriptor are not yet available. A different alternate setting can
reuse an existing interface *number*, but cannot turn `0x00`,
`0x01` and `0x02` into the same interface number.
A raw configuration header is needed to independently confirm
`bNumInterfaces` and every alternate-setting/endpoint association.

`bNumEndpoints=0x03` counts **three non-control endpoints** for
the specified interface/alternate setting, **excluding EP0**.
It does **not** identify endpoint address 3, transfer direction,
interrupt/bulk type, packet size, scanner operation, fax transport,
or the total endpoint count of the whole device. Those fields require
actual endpoint descriptors (`bEndpointAddress`, `bmAttributes`,
`wMaxPacketSize`, and `bInterval`) and careful association
with the correct interface.

Brother's Linux MFC-290C downloads document separate scanning software.
Published `brscan3` scanner model lists include `0x01fd` for
MFC-290C, and publicly reproduced Brother scanner `udev` rules
match `ff/ff/ff` Brother USB interfaces. Taken together, these
make **interface `0x01` a credible scanner candidate**, *not a
verified scanner mapping*: those rules are family-wide, and the
scanner's exact interface number and transfer protocol remain
unknown. Do not copy scanning or PC-FAX opcodes from another
model, infer bulk/interrupt assignments, or take exclusive USB
interface control.

**Interface `0x02`: candidate mass-storage signature.**
The model also documents Macintosh file access to inserted media.
A **standard USB Mass Storage SCSI-transparent Bulk-Only**
interface would advertise `class=0x08 subclass=0x06 protocol=0x50`
and typically require bulk IN/OUT endpoints. Apple's historical
mass-storage driver documentation records this exact subclass
and protocol combination. The offline classifier and macOS
probe now recognize this combination as
`protocolMeaning=storage-scsi-bulk-only`, but **we have no
observation that interface 2 actually reports it**. Do not
equate the machine's front USB-host port with this hypothetical
computer-facing device interface. If `0x02` instead reports
`ff/ff/ff` or another triplet, keep the role unassigned.

For the reported device `04f9:01fd`, `idVendor=0x04f9` identifies
**Brother**. All-FF in the *interface* fields means the class and
its subclass/protocol are **manufacturer-defined** in this context.
It does **not** mean an FF-coded USB command, nor that all USB
SETUP requests are vendor requests: standard USB enumeration and
descriptors still apply. It does not identify a Brother command
opcode, an ASIC architecture, a scanner interface or a fax endpoint.
A vendor-specific interface may use multiple transfer types and
vendor-controlled or standard request sequences.

Apple's archived *USB Device Overview* describes interface-specific
matching for class FF using `idVendor + bInterfaceSubClass +
bInterfaceProtocol` and the more targeted `idVendor + idProduct +
bConfigurationValue + bInterfaceNumber`. The latter combination
avoids binding every Brother FF/FF/FF interface the same way;
these are **historical matching rules**, not a modern DriverKit
installation design.

According to USB-IF *USB Device Class Definition for Printing Devices*
version 1.1, section 5.3, `class=0x07 subclass=0x01` identifies a
printer interface; protocols `0x01`, `0x02`, `0x03` mean unidirectional,
bidirectional and IEEE 1284.4-compatible respectively. In that specific
printer-class context, `protocol=0xff` means **vendor-specific printer
protocol**, for which the class-specific printer protocols do not apply.
In contrast, `class=0xff` defines a **vendor-specific interface class**
independently of the protocol byte. Neither proves the endpoint roles,
actual Brother print-language bytes, scanner role, fax role or ASIC ISA.

The existing host-only `whp_mfc290c_usb_hint_interface()` now
classifies **only known combinations** without emitting USB commands:
legacy printer protocols 1/2/3, printer vendor protocol FF, vendor
class FF, or unclassified. Other pairs, including newer protocol
extensions, remain unclassified rather than being falsely labeled.
The macOS probe prints `protocolMeaning=printer-vendor-protocol`
only when all three validated properties are present and match
`07/01/ff`; the reported `ff/ff/ff` produces `protocolMeaning=vendor-class`;
missing fields produce `protocolMeaning=insufficient`.
The field is a **class-definition interpretation of registry metadata**
and does not establish a functional driver or authorize printer class
GET_DEVICE_ID, status, soft reset or other class-specific requests.

The original offline SETUP classifier takes *class alone*, and its
printer-specific `*_CANDIDATE` result is only a **syntactic**
classification of captured traffic. When a verified interface
advertises vendor-specific `07/01/ff`, do not use that candidate
as evidence that standard printer class requests are accepted.
Never apply these interpretations across separate interface numbers.

**Non-mutating follow-up:** the newly known printer interface is
`number=0x00`. Capture the **full interface `0x02` line** with
`class`, `subclass`, `protocol`, `alternate`,
`declaredEndpoints` and `protocolMeaning`, and the remaining
`alternate` values for interfaces `0x00`/`0x01`.
Read-only registry metadata cannot by itself reveal endpoint
addresses; use an authenticated raw configuration descriptor for
that later. Keep the machine's serial number private. Do not
issue vendor requests, switch alternate settings, detach drivers
or reset the printer.

Sources:
- USB-IF, *Universal Serial Bus Common Class Specification* revision 1.0,
  §3.10–3.11 (vendor-specific interface matching and vendor request
  identification):
  https://www.usb.org/sites/default/files/usbccs10.pdf
- Apple archived *USB Device Overview*, interface matching Table 1-3:
  https://developer.apple.com/library/archive/documentation/DeviceDrivers/Conceptual/USBBook/USBOverview/USBOverview.html
- USB-IF, *USB Device Class Definition for Printing Devices*,
  version 1.1, §5.3, interface descriptor Table 6:
  https://www.usb.org/sites/default/files/usbprint11a021811.pdf
- Apple IOKit `bInterfaceProtocol` descriptor field:
  https://developer.apple.com/documentation/kernel/iousbinterfacedescriptor/1545890-binterfaceprotocol
- Microsoft USB interface descriptor documentation (endpoint count,
  interface number and alternate-setting fields):
  https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/usbspec/ns-usbspec-_usb_interface_descriptor
- Microsoft USB configuration descriptors (interface and endpoint
  relationships):
  https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/usb-configuration-descriptors
- Brother MFC-290C Linux support downloads (scanner context, not
  interface mapping evidence):
  https://support.brother.com/g/b/downloadlist.aspx?c=us_ot&lang=en&os=128&prod=mfc290c_all
- Published `brscan3` model config and Brother `udev` scanner
  rules (secondary/community reproductions, **not** an
  authenticated MFC-290C USB descriptor):
  https://forums.opensuse.org/t/brother-scanner/115077
  https://askubuntu.com/questions/1530593/brother-scanner-missing-start-up-error-messages
- Apple's historical USB mass storage class binding rules:
  https://leopard-adc.pepas.com/documentation/DeviceDrivers/Conceptual/MassStorage/04_Matching/MS_Matching.html
- USB-IF Mass Storage class information:
  https://www.usb.org/defined-class-codes

## Apple macOS open-source integration audit (2026-11-21)

**Source pinning and scope:** Apple's published system components are
valuable for **how the host enumerates and binds USB printers**, but
they do **not** provide the MFC-290C's proprietary print raster format,
scanner transport, PC-FAX job language, flash contents or ASIC ISA.
Do not treat historical IOKit classes as modern Apple Silicon driver
APIs or mistake macOS framework documentation for published source.

| Apple reference | Verified content | MFC-290C consequence |
| --- | --- | --- |
| `apple-oss-distributions/cups`, `cups/backend/usb-darwin.c` | Historical macOS CUPS USB backend: printer-class interface matching; class-plugin binding; IEEE 1284 GET_DEVICE_ID; USB device-descriptor and string fallback; serial and location selection | Inspect host-side binding and identity **without** copying a full backend or claiming proprietary Brother output encoding |
| `cups/backend/usb.c` | Selects macOS `usb-darwin.c` unless configured with libusb | Do not assume that every newer macOS CUPS installation uses this exact historical backend path |
| `cups/backend/org.cups.usb-quirks` | Contains older Brother HL-1250/1430/1440/1450 entries and no exact `0x04f9:0x01fd` entry in reviewed file | No MFC-290C-specific reset, unidirectional or no-reattach quirk is established |
| `apple-oss-distributions/IOUSBFamily`, `IOUSBFamily/Headers/IOUSBLib.h` | Historical `GetInterfaceClass`, `GetAlternateSetting`, `GetNumEndpoints`, `GetLocationID`, USB serial string index; read-only accessor calls do not all require interface-open | Confirms intent of our non-mutating observer, but this vintage API is **not** the modern DriverKit interface |
| XNU / IOKit open components | General IORegistry and kernel-driver interface architecture | Supports driver-boundary design, not Brother-specific USB protocol or firmware |
| ImageCaptureCore / USBDriverKit | Apple SDK documentation for scanning and supported modern USB driver interfaces | Investigate scanner integration and signed USB drivers **separately** after real interface metadata and entitlements are established |

**Identity source separation:** The legacy CUPS backend's
`copy_printer_interface_deviceid` retrieves an **IEEE 1284 printer ID**
through a USB printer-class control request (including manufacturer,
model and sometimes `SERN`). It separately falls back to standard USB
device and USB **string descriptors** (`iManufacturer`, `iProduct`,
`iSerialNumber`) if needed. These are distinct encodings and retrieval
paths; a CUPS queue's model string and URI serial are not direct proof
of a device's raw USB `iSerialNumber` index. The user's previously
reported System Information serial is **not embedded** in the project.

The Apple backend's `GET_DEVICE_ID` performs actual USB I/O. Our
existing `mfc290c_usb_commands` module only *classifies captured*
printer requests and does not send them. Do not call the old backend
directly from an observer, steal a claimed USB interface, or issue
reset/print/scan/fax commands merely to obtain metadata.

**New passive diagnostic:** the macOS IOKit probe checks for Apple's
historical interface property `USB Printing Class` (the CUPS source's
`kUSBClassDriverProperty`) and reports only
`legacyCupsPlugin=present` or `legacyCupsPlugin=unavailable`.
Neither the plugin's absolute path nor its contents are printed.
Presence is an historical binding hint **only**; absence is not proof
that no current print path exists. The check does not load the plugin,
open the interface, enumerate a new descriptor or send USB traffic.

**Most useful next evidence:** read-only, sanitized actual MFC-290C
USB device/configuration/interface/endpoint descriptors, plus
driver/class service hints. With that evidence, compare printer-class
binding to CUPS support and separately inspect ImageCaptureCore for
the scanner. Only then choose a current macOS print/scan integration
without assuming DriverKit is needed for all functions.

Frozen source references (source revision to prevent silent drift):
- Apple CUPS: https://github.com/apple-oss-distributions/cups/tree/2716564fcd1f5bf4cd4082279e97625a8f98204e
- macOS USB backend: https://github.com/apple-oss-distributions/cups/blob/2716564fcd1f5bf4cd4082279e97625a8f98204e/cups/backend/usb-darwin.c
- CUPS quirks: https://github.com/apple-oss-distributions/cups/blob/2716564fcd1f5bf4cd4082279e97625a8f98204e/cups/backend/org.cups.usb-quirks
- Historical IOUSBLib: https://github.com/apple-oss-distributions/IOUSBFamily/blob/main/IOUSBFamily/Headers/IOUSBLib.h
- Apple ImageCaptureCore: https://developer.apple.com/documentation/imagecapturecore
- Apple USBDriverKit: https://developer.apple.com/documentation/usbdriverkit

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

**Verified in CI:** C11 host tests, macOS IOKit/CUPS observer compilation,
Universal 2 Mach-O slices and read-only smoke tests have passed on
macOS 15; portable tests also passed on Ubuntu. **Not verified on
the physical MFC-290C:** current IOKit/CUPS observations, raw endpoint
descriptors, Brother print/scan/fax command streams, modern ICA support
or actual printing/scanning/media access. CI has no attached printer.

## Windows INF FeatureScore and PDD resource mapping (2026-11-21)

**Updated user-supplied observations:** The user has extracted a Brother
Windows MFC-290C driver package, read an **INF file that references a
PDD-like file**, and observed the literal directive
`FeatureScore=0xFE`. The user subsequently **corrected the earlier
filename report as containing a typo**, but has not supplied the
corrected name or INF excerpt. Therefore the formerly reported
`BRMF250C.PDD` is **not a verified filename in this particular
extraction**. Do not hard-code it, rename files based on it, or
claim the MFC-290C loads that resource. Actual INF section,
file bytes, paths, driver version, INF model matching and installed
configuration remain **uninspected**; the user-reported
`FeatureScore=0xFE` is the new confirmed clue.

A published **third-party file inventory** of a Brother 2009-era
multi-model inkjet Windows driver lists *both*
`brmf250c.pdd` / `brmf250c.ini` and
`brmf290c.pdd` / `brmf290c.ini` alongside shared
`brpri07a.inf`, `bril08b.dll`, `brio08b.dll`,
`briu08b.dll` and `.bcm` files. This is evidence of
**multiple model-named resources distributed together**,
not verification of the user's particular bundle or of the
function each binary serves. **Check first for the actual file names and references in the
user's extracted INF**, including any `BRMF290C.PDD` if present,
then inspect its model-specific install section and companion
INI references to determine which resources and driver DLL
are used.

### Meaning of the observed `FeatureScore=0xFE`

Microsoft's [INF FeatureScore directive](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/inf-featurescore-directive)
is a **Windows Vista-and-later driver-selection rank**, processed
**only when directly present in a relevant `[DDInstall]` section**.
The single-byte score is `0x00`–`0xFF`, where **lower is preferred**
and an **omitted score defaults to `0xFF`**. The observed
`0xFE` contributes `0x00FE0000` to a composite rank formatted
`0xSSGGTHHH` (signature score, feature score, device-ID match score).
This is a **slight preference against an otherwise comparable
default-`0xFF` candidate**, not a guarantee of driver selection.
Windows also considers signature and hardware/compatible ID matching;
driver date/version can then influence selection. The fact that
a directive is present does **not** prove the section belongs to
the MFC-290C printer function or is effective on Windows XP.

**The crucial contextual check** is which section contains the
`FeatureScore` and whether the MFC-290C model row actually points
there on the target Windows architecture/OS. Inspect the exact
`[Version]` and `Class`, `[Manufacturer]`, decorated
`[Models]` entries, device IDs, `[DDInstall]`/
`[DDInstall.NT...]` variants, `CopyFiles`, `Include`,
`Needs`, `AddReg`, `[SourceDisksFiles]` and
`[DestinationDirs]` references. A file **listed for copying**
is not by itself proof that the selected print pipeline reads it.
If a companion scanner INF has its own section, investigate that
separately; printer/scanner/PC-FAX installs must not be merged
because this is one all-in-one machine. No authentic INF excerpt
has yet been inspected, so the specific installed function and
effective rank remain unknown.

**Driver boundary:** `0xFE` is not USB endpoint `0xFE`,
a firmware revision, print resolution, ink capability, scanner
mode or a vendor request. This directive is useful as a map
through Windows **installation wiring** toward real binary/data
resources; it does not itself reveal the Brother protocol.
Cross-check resource names with MFC-290C INF model rows before
porting any independently validated settings to macOS.

References:
- Microsoft FeatureScore directive:
  https://learn.microsoft.com/en-us/windows-hardware/drivers/install/inf-featurescore-directive
- Microsoft package-ranking format and significance:
  https://learn.microsoft.com/en-us/windows-hardware/drivers/install/how-windows-ranks-driver-packages
- Microsoft INF DDInstall section structure:
  https://learn.microsoft.com/en-us/windows-hardware/drivers/install/inf-ddinstall-section
- Microsoft driver package selection:
  https://learn.microsoft.com/en-us/windows-hardware/drivers/install/overview-of-the-driver-selection-process

### Meaning of the observed `Needs=sRGBPROFILE.OEM` (2026-11-21)

**New user-observed Brother Windows INF line:**
`Needs=sRGBPROFILE.OEM`. Microsoft's *Printer INF File Install
Sections* explicitly defines **`[sRGBPROFILE.OEM]`** in the
Windows **`NTPRINT.INF`** core printer INF as the section
that **installs the Windows system sRGB color profile**.
This is a **Windows printer installation dependency**, not
evidence of a Brother-specific ICC profile, printhead color
register, USB request or color-fax data path.

An INF references **external named install sections** using
`Include=NTPRINT.INF` with `Needs=...`, or, on supported
Windows versions, package-aware **`CoreDriverSections`**.
The user's actual `Include` line, installation-section
header, associated printer model row, target Windows version
and any local `[sRGBPROFILE.OEM]` section have **not been seen**.
Therefore the strongest supported reading is a conventional
reference to Microsoft's standard print-profile section;
do not assert that it ran for the MFC-290C without tracing
the device's **model → selected DDInstall → included section**
on the relevant Windows architecture. The literal `Needs`
token is the user's evidence; `Include=NTPRINT.INF` is
**expected context, not an observed line**.

**Do not merge color-related layers:**

| Layer | What is supported | What remains unknown |
| --- | --- | --- |
| Windows print installation | Standard system sRGB ICC profile install-section exists in `NTPRINT.INF` | Whether the extracted Brother INF selects the section for MFC-290C |
| Printing color management | Windows can use an ICC color profile; MFC-290C prints in color | Which profile is actually assigned, any Brother `.icm`/`.icc`/conversion tables, and host-versus-firmware RGB-to-ink processing |
| Scanning | Brother model has a color CIS and supported host scanning | Whether its WIA/TWAIN package assigns a scanner-specific ICC profile; WIA scanner profile handling is separate |
| Standalone color fax | Brother advertises color fax; telecom-side JPEG/color encoding is separately documented | No inference about fax protocol, scan-to-fax color management, or PC-FAX color support from this printer INF |
| macOS printing | Apple ColorSync provides ICC profile APIs and color-space conversions | A usable MFC-290C output profile, native raster encoder, actual USB print data and validated print result |

Microsoft's printer *Installing ICC Profiles* documentation
shows printer `CopyFiles` sections for `.icm` profiles and
`DestinationDirs` profile dirid **66003**; look for
`ICMProfiles`, `CopyFiles`, `.icm`, `.icc`, `AddReg`,
`DataFile` and `CoreDriverSections` in the model's
relevant install sections. A generic system sRGB input/workflow
profile must **not** be silently represented as the MFC-290C's
calibrated ink/paper *output* profile. Similarly, the presence of
this line does not prove a particular Windows GDI, Pscript or
Unidrv rendering backend; cross-check the model's `DataFile`,
`DriverFile`, `DataSection` and `Needs` chain.

**Safe static inspection of the already extracted file:**

```sh
# Use the real path to the extracted INF; this reads but never installs it.
grep -nEi '^[[:space:]]*\[|FeatureScore|Needs|Include|CoreDriverSections|sRGBPROFILE|ICMProfiles|\.icm|\.icc|CopyFiles|DataFile|DriverFile|MFC[-_ ]?290C' path/to/driver.inf
find . -type f \( -iname '*.icm' -o -iname '*.icc' \) -print
```

Review the *entire section* containing `Needs=sRGBPROFILE.OEM`,
then follow its actual model mapping. Read-only parsing or
hashing can later test any authentic ICC profile; do not run
the Windows installer, upload proprietary binary profiles to
the repository, or change a Mac-wide color configuration
from these historical INF findings. First identify which
device-function INF this belongs to and distinguish a
generic installed sRGB profile from an authentic Brother
output profile.

Authoritative references:
- Microsoft printer INF core install sections
  (`sRGBPROFILE.OEM` and `NTPRINT.INF`):
  https://learn.microsoft.com/en-us/windows-hardware/drivers/print/printer-inf-file-install-sections
- Microsoft INF DDInstall `Needs` and `Include` context:
  https://learn.microsoft.com/en-us/windows-hardware/drivers/install/inf-ddinstall-section
- Microsoft printer ICC profile installation, profile directory 66003:
  https://learn.microsoft.com/en-us/windows-hardware/drivers/print/installing-icc-profiles
- Microsoft WIA scanner INF profiles (independent of printer install):
  https://learn.microsoft.com/en-us/windows-hardware/drivers/image/inf-files-for-wia-devices
- Apple ColorSync framework (macOS color management):
  https://developer.apple.com/documentation/colorsync

### Scanner INF: observed `SubClass=StillImage` (2026-11-21)

**New user observation:** A *different* extracted Brother Windows
INF file contains `SubClass = StillImage`. Treat the exact
directive as evidence from the user's package, but **not yet
as a verified MFC-290C-specific install section**: this INF's
filename, `[Version]`, section header, hardware-ID model
row and other directives have not been supplied. Keep it
**separate from the printer INF** that contains
`FeatureScore=0xFE` and `Needs=sRGBPROFILE.OEM`.

Microsoft documents `SubClass=StillImage` as a **required
STI/WIA-compatible still-image installation entry** processed
by the `sti_ci.dll` class installer when it is in a
relevant `[DDInstall]` section. It can describe more than
scanners: **`DeviceType=1` means scanner,
`2` camera, and `3` video**. Until `DeviceType`
and the MFC-290C `[Manufacturer]`/`[Models]` mapping
are checked, this line is strong evidence of an **imaging**
installation path, not proof of a model-specific scanner
binding.

| INF key or reference | What to validate in *this* INF |
| --- | --- |
| `[Version]`, `Class=Image`, `ClassGuid` | Image setup class and exact package identity |
| `[Manufacturer]` and decorated model sections | MFC-290C model string and Windows PnP hardware ID; match `04F9:01FD` and any `MI_01` suffix only if actually present |
| `SubClass=StillImage`, `DeviceType=1` | Legacy STI/WIA scanner install classification; `DeviceType` has **not** been observed |
| `Include=sti.inf` with `Needs=STI.USBSection` | Documented Windows USB still-image DDInstall pattern; **not yet observed here** |
| Corresponding `.Services` section | Possible `sti.inf` / `STI.USBSection.Services` chain and any actual kernel driver registration |
| `DeviceData`, `AddReg`, `USDClass`, `MicroDriver`, `TwainDS`, `CopyFiles` | Candidate WIA/TWAIN registrations and vendor binaries, conditional on real entries |
| `Events`, `ICMProfiles` | Possible scan-button event registrations or color-profile metadata—neither yet observed |

### Scanner INF: decoding observed `Capabilities=0x39` (2026-11-21)

**New user-reported Windows INF clue:** `Capabilities = 0x39`
in the ongoing extracted imaging/possible scanner-driver investigation.
The underlying INF's *actual lines* have not been provided, so
the exact DDInstall section and MFC-290C model binding remain
unverified. Microsoft documents `Capabilities` in the
Still Image (STI)/Windows Image Acquisition (WIA) installation
metadata as a bitmask whose flags populate
`STI_DEV_CAPS.dwGeneric` for Windows imaging components.
`0x39` is decimal **57**, binary **00111001**:

| Bit | Mask | Flag | Status in 0x39 and correct interpretation |
| --- | --- | --- | --- |
| 0 | `0x01` | `STI_GENCAP_NOTIFICATIONS` | **Set:** Windows driver advertises STI event notifications |
| 1 | `0x02` | `STI_GENCAP_POLLING_NEEDED` | **Clear:** no mandatory polling declared; not proof of functional asynchronous USB events |
| 2 | `0x04` | `STI_GENCAP_GENERATE_ARRIVALEVENT` | **Clear:** no STI-generated device-arrival launch event requested through this bit |
| 3 | `0x08` | `STI_GENCAP_AUTO_PORTSELECT` | **Set:** legacy Windows auto COM/LPT port-selection metadata; **not** evidence of a serial/modem USB interface |
| 4 | `0x10` | `STI_GENCAP_WIA` | **Set:** imaging driver claims Windows WIA compatibility |
| 5 | `0x20` | `STI_GENCAP_SUBSET` | **Set:** legacy subset-driver flag; Microsoft's WIA INF guidance says bit 5 is **currently not used**. Do not infer a missing hardware feature |

**Comparison control:** Microsoft examples document `0x31`
for notification-capable interrupt-style WIA events and
`0x33` for notification handling with **polling required**.
The observed `0x39` has bits `0x01` and `0x10`
in common, but additionally `0x08` and `0x20` and
**does not have bit `0x02`**. That distinction makes the
Brother scanner's **notification path** worth tracing
through the selected WIA minidriver and `Events`
registry entries. It does **not** prove an interrupt
USB endpoint, a physical scan-button message, a polling
interval, CIS/ADF source-selection commands, or successful
WIA interoperability on a modern Windows installation.

**Safe next verification:** Trace `[Manufacturer]` /
`[Models]` hardware IDs (including `04F9`, `01FD`
and any actual `MI_01` suffix) to the **specific**
`[DDInstall]` containing `SubClass=StillImage`,
`DeviceType`, `Capabilities`, `DeviceData`, and
`Events`. Confirm any `Include=sti.inf`,
`Needs=STI.USBSection`, `USDClass`, `MicroDriver`,
`TwainDS`, referenced DLLs and `CopyFiles` entries
**from that actual file**; do not invent missing lines.
Compare registered event GUIDs, `IStiUSD::GetCapabilities`
and any `GetNotificationData` handling in legitimately
inspectable Windows driver code. On macOS, a native
ImageCaptureCore/ICA adapter can reuse verified
**user-facing event semantics**, but not these Windows
STI numeric bits or Windows COM DLLs as a direct ABI.

**Security and hardware guard:** This is a read-only
interpretation of Windows software metadata, not a
driver implementation or evidence of new peripheral
silicon. Do not transmit speculative USB status requests,
move the scanner, alter firmware, send faxes, or
publish proprietary installer binaries for the purpose
of decoding this flag.

References:
- Microsoft STI/WIA INF bit assignments and bit-5 caveat:
  https://learn.microsoft.com/en-us/windows-hardware/drivers/image/inf-files-for-wia-devices
- Microsoft STI device capabilities and legacy auto-port semantics:
  https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/sti/ns-sti-_sti_dev_caps
- Microsoft scanner polling event example (`Capabilities=0x33`):
  https://learn.microsoft.com/en-us/windows-hardware/drivers/image/adding-polling-event-support
- Microsoft scanner interrupt event example (`Capabilities=0x31`):
  https://learn.microsoft.com/en-us/windows-hardware/drivers/image/adding-interrupt-event-support

### Scanner INF: observed `EventEmail=1` and host-assisted email (2026-11-21)

**New user report:** Another line in the extracted Brother imaging
INF reads `EventEmail=1`. The exact INF section, original
`AddReg` or `DeviceData` relationship, associated event GUID,
registry key/value type, driver version and MFC-290C model-row
binding **have not been supplied**. Preserve the literal line as
an **observed filename-independent software clue**, not a
Microsoft-defined standard INF directive with a verified binary
meaning. The provisional inference is a Brother-specific
**email-related scan-event option or registration**, not
proof of functioning hardware notification delivery.

**Independent model confirmation:** Brother's model-specific
ControlCenter2/3 FAQ identifies **Scan to E-mail** as
scanning a document into an attachment in a selected **computer
email application**. It distinguishes the software-button
configuration from the physical machine's **Device Button**
configuration. Brother's legacy Software User's Guide for the
MFC-290C family describes pressing **Scan → Scan to E-mail**
with a USB cable and host-side MFL-Pro/ControlCenter software
installed. Brother's published MFC-290C product specifications
give a **USB** PC interface, not an Ethernet or Wi-Fi network
interface for independent mail delivery.

```text
MFC-290C flatbed/ADF + Scan control
    -> MFC-290C USB device link (protocol/event packets unknown)
    -> Windows WIA/TWAIN + Brother ControlCenter3
       or historical Mac scanner + ControlCenter2
    -> scanned file / selected host email application attachment
    -> computer's own network connection for optional send
```

**Do not conflate three different features:**

| Function | Supported inference | Unsupported inference |
| --- | --- | --- |
| Scan to E-mail (PC) | Brother documents handing an image/file attachment to a host mail application | Printer has its own SMTP stack, Ethernet NIC or Wi-Fi radio |
| `EventEmail=1` in scanner INF | Plausible event or email-workflow registration, to be traced to an actual device-specific INF section | A defined Microsoft `EventEmail` keyword, USB opcode, endpoint, physical button packet, or automatic sending |
| Analog fax over `LINE` / `EXT.` | Separate standalone telephone-line fax service and PC-FAX sending where documented | Fax transmission equals Internet email or Scan-to-Email |

**Next read-only INF tracing:** Find the line's **section
header**; examine nearby `Events=`, `DeviceData=`,
`AddReg=`, event names/GUIDs and associated `USDClass`,
`MicroDriver`, `TwainDS`, `CopyFiles` or
ControlCenter binaries. Microsoft's STI/WIA documentation defines
the event-section row grammar
`EventName="Display", {GUID}, App`; it does **not**
document `EventEmail` as a universal keyword. The line might
appear in a **vendor data section**, in which case its meaning
must be recovered from the actual Brother code/registry
consumer. Cross-check `SubClass=StillImage`,
`Capabilities=0x39` and the selected MFC-290C
hardware-ID entry, but do not assume the three lines
are in one effective install section.

For a native macOS scanner, treat **Scan to E-mail** as
a user-invoked *host application handoff*: receive the scan
using independently verified Brother USB scanning commands,
write a safe local document, and let the chosen mail
application create a draft with an attachment. **Never
automatically send mail**, select recipients, or infer
SMTP credentials from this INF line. WIA event flags and
Windows COM interfaces are **not portable** macOS APIs.
Scanner-button notifications and direct scan data remain
unimplemented.

Sources:
- Brother MFC-290C ControlCenter2/3 scanning and email options:
  https://support.brother.com/g/b/faqend.aspx?c=as_ot&faqid=faq00002561_000&ftype3=1993&lang=en&prod=mfc290c_all
- Brother MFC-290C ControlCenter E-mail application and separate
  Software Button/Device Button settings:
  https://support.brother.com/g/b/faqend.aspx?c=as_ot&faqid=faq00002614_000&ftype3=2036&lang=en&prod=mfc290c_all
- Brother model-specific interface specifications:
  https://support.brother.com/g/b/spec.aspx?c=us&lang=en&prod=mfc290c_all
- Microsoft, WIA INF device event sections and DeviceData:
  https://learn.microsoft.com/en-us/windows-hardware/drivers/image/inf-files-for-wia-devices
- Microsoft, push-model aware applications and INF event registration:
  https://learn.microsoft.com/en-us/windows-hardware/drivers/image/creating-push-model-aware-applications

### Scanner INF: `BrScnUsb` function-service lead (2026-11-21)

**New exact user-observed line from the extracted Windows scanner
INF:** `AddService = BrScnUsb, 0x00000002,
BrScnUsb.AddService`. This is materially stronger than a
feature flag: Microsoft's documented `AddService` grammar
makes `BrScnUsb` the **service name**, `0x00000002`
the **`SPSVCINST_ASSOCSERVICE`** flag, and
`BrScnUsb.AddService` a **service-install section
reference**, not an installed binary filename.
The flag associates the named service with the device's
**Plug and Play function driver** when that INF's
`[DDInstall.Services]` entry is selected. It is
**not** a USB interface number, subclass, firmware
value, Win32 service startup type or guarantee that
the Windows driver actually installed on the user's host.

**Cross-model software corroboration:** Independent older Windows
inventories identify `BrScnUsb.sys` as a *Brother USB
Scanner Driver*; inventories of another Brother MFC family
package contain that file. **Neither verifies** the exact
MFC-290C package's binary name, hash, architecture or version.
We must read `ServiceBinary` from the actual
`[BrScnUsb.AddService]` section before promoting
`BrScnUsb.sys` from a comparative candidate to
a supported MFC-290C driver dependency.

**Targeted read-only mapping, in order:**

1. Locate the **actual model row**, e.g. the MFC-290C
   hardware-ID match, and follow its chosen decorated
   `[DDInstall]` then `[DDInstall.Services]` section.
   Do not transplant a service from another Brother model
   or conflate the printer INF with this scanner INF.
2. Read the exact `[BrScnUsb.AddService]` section:
   `ServiceType`, `StartType`, `ErrorControl`,
   `ServiceBinary`, `DisplayName`, `AddReg`,
   `Dependencies` and `LoadOrderGroup` if present.
   In particular, `ServiceType=1` would identify a
   kernel driver; no such value has yet been observed.
3. Follow the named binary through `CopyFiles`,
   `SourceDisksFiles` and `DestinationDirs`.
   Record filename, version, selected architecture,
   byte size and cryptographic hash **from the real
   extracted file**. Do not assume `%12%` or a
   `.sys` extension before inspecting `ServiceBinary`.
4. Analyze **offline**, not by loading the legacy
   Windows driver: inspect PE/COFF headers, imports/exports,
   recognizable IOCTL names and any WIA/STI/TWAIN
   dependencies using non-executing tools. Even if
   `BrScnUsb.sys` proves to be the function driver,
   **its commands, IOCTL layout and USB endpoint binding
   remain unrecovered**.
5. Correlate the Windows hardware-ID match and service
   with the physical device's `04f9:01fd`
   descriptors. An `MI_01` suffix, if observed,
   would be a separate software-binding hint, not
   proof of endpoint roles without raw descriptors.
   Preserve the shared CIS, flatbed and ADF capability
   boundaries, and the host-assisted scan-to-email
   distinction already documented.

**Safe local inspection**, using the real extracted INF path
and its actual text encoding:

```sh
grep -nEi '^\[|AddService|BrScnUsb|ServiceType|StartType|ErrorControl|ServiceBinary|CopyFiles|SourceDisksFiles|DestinationDirs|VID_04F9|PID_01FD|MFC[-_ ]?290C' path/to/scanner.inf
find . -type f -iname 'brscnusb*' -print
```

**macOS design:** This reveals a Windows *driver-stack
entry point*, not code directly portable to Apple Silicon.
We can reuse only independently verified device protocol
semantics in a native scanner implementation using
ImageCaptureCore/ICA or another supported macOS interface.
No speculative USB controls, device resets, scans,
firmware writes, binary execution or privileged driver
installation are introduced by this research.

References:
- Microsoft AddService directive and
  `SPSVCINST_ASSOCSERVICE`:
  https://learn.microsoft.com/en-us/windows-hardware/drivers/install/inf-addservice-directive
- Microsoft service-install entries and `ServiceBinary`:
  https://learn.microsoft.com/en-us/windows-hardware/drivers/install/hklm-system-currentcontrolset-services-registry-tree
- Brother model's Windows scanner-driver download listing:
  https://support.brother.com/g/b/downloadlist.aspx?c=us&lang=en&os=6&prod=mfc290c_all
- Cross-model older Windows scanner-driver inventory
  (not proof of the user's package):
  https://www.nirsoft.net/utils/csv_file_comparison.html
- Cross-model Brother driver-package file inventory
  (not proof of MFC-290C binding):
  https://www.runonpc.com/driverdownload/brother-mfc-j430w-driver-download/

### Scanner INI: observed `Model=23` (2026-11-21)

**New physical-software evidence supplied by the user:** A Windows
driver-package INI file that the user associates with the MFC-290C
contains the literal text `Model=23`. This exact value is an
**observed, unclassified vendor configuration setting**, not a
documented USB request, Windows INF directive, USB interface
number, chip part number, physical scanner count, firmware
revision or scanner resolution. The actual INI filename, parent
section (`[...]`), surrounding entries, file encoding, package
revision, byte contents and the component that **reads** the
setting have **not yet been supplied or independently inspected**.
The user describes the file as tied to the MFC-290C; that is a
reported association, **not yet a verified code-level binding**.

Unlike Windows standard `Capabilities=0x39` or
`AddService=BrScnUsb`, the meaning of this `Model`
key is not established by a public Microsoft or Brother
format contract. Treat the characters `23` initially as
a decimal-looking **literal value**; do not assume a
decimal index versus hexadecimal code, enum, bitmap,
USB product identifier or product-generation date without
evidence from its parser. A speculative numeric conversion
(`23` decimal = `0x17`) adds **no evidence**.

**Candidate interpretations to test, not facts:**

| Hypothesis | Evidence required before acceptance |
| --- | --- |
| Internal Brother scanner model/profile ID | Exact INI section, MFC-290C-specific folder/installer mapping, and authenticated code consuming `Model` |
| Model-family feature profile (CIS, flatbed, ADF, color, media) | Neighboring model INI files with comparable settings plus real behavior/published features, and a confirmed parser |
| Index into data/resources | Code reference or table mapping `23` to a named resource; do not guess an offset |
| Windows device-install identifier | Actual INF `[Models]` hardware-ID reference and established mapping; **distinct from `04f9:01fd`** |
| Hardware controller/chip identifier | Exact component marking and manufacturer datasheet or authenticated firmware/software mapping; a bare INI key is insufficient |

**Highest-value read-only correlation work:**

1. Preserve original INI **filename, path, package revision and
   SHA-256**. Determine encoding using `file` and a short
   `hexdump`; standard text `grep` may miss UTF-16 INI data.
2. Record the **section header immediately above** `Model=23`
   and nearby keys, particularly names indicating the
   scanner source, WIA/TWAIN driver, model name, feature flags,
   product ID, driver DLL, or USB transport. Do not infer
   that the surrounding values all apply to one USB interface.
3. Search the extracted **INF** for this INI filename and
   associated `CopyFiles`, `SourceDisksFiles`,
   `DestinationDirs` and `AddReg` references. A copied
   file is **not** proof that it is loaded at runtime.
4. Identify the actual executable/DLL that **opens/parses**
   the INI through trustworthy static code/data references.
   This may be a WIA/TWAIN/ControlCenter component distinct
   from the `BrScnUsb` function-driver service; the
   service name alone does not identify the consumer.
5. If available, compare **MFC-250C, MFC-290C and other
   service-manual-family** INI files from **same driver
   release**. Differences can discriminate model-specific
   options from family-common keys; never transfer values
   merely because neighboring products share a controller
   or installer.
6. Cross-check any claimed feature or mechanism with the
   existing documented **single CIS + separate flatbed/ADF
   mechanics**, the **`04f9:01fd`** USB identity, and
   independently verified endpoint descriptors. Only
   then consider an offline format decoder or a portable
   macOS capability table.

**Non-executing macOS inspection (choose real paths):**

```sh
find . -type f -iname '*.ini' -print
file path/to/confirmed.ini
shasum -a 256 path/to/confirmed.ini
hexdump -C -n 128 path/to/confirmed.ini
# For confirmed single-byte/ASCII-compatible text only:
grep -nEi '^\[|^[[:space:]]*Model[[:space:]]*=|MFC[-_ ]?290C|04F9|01FD|TWAIN|WIA|ADF|USB' path/to/confirmed.ini
```

If the INI is UTF-16, convert a **read-only copy** to UTF-8
before searching; keep the original file and hash intact.
Do not run/install its legacy Windows binaries, publish
proprietary package files, issue USB requests, move the
scanner, dial faxes or write firmware to investigate it.

**Status:** A candidate link between an MFC-290C software
configuration and the existing WIA/TWAIN + `BrScnUsb`
investigation. **No verified `Model=23` semantic
mapping, device-model lookup table, new scanner chip,
new USB opcode or macOS driver behavior yet.** The key
research target is the **INI consumer plus its section
context**, not the integer by itself.

Supporting context (not documentation of `Model=23`):
- Brother MFC-290C official product specifications
  (WIA/TWAIN and hardware features):
  https://support.brother.com/g/b/spec.aspx?c=us&lang=en&prod=mfc290c_all
- Brother official MFC-290C Windows scanner-driver download:
  https://support.brother.com/g/b/downloadlist.aspx?c=us&lang=en&os=6&prod=mfc290c_all
- Microsoft WIA INF `DeviceData` and vendor-settings guidance:
  https://learn.microsoft.com/en-us/windows-hardware/drivers/image/inf-files-for-wia-devices
- Microsoft still-image INF model-sharing documentation:
  https://learn.microsoft.com/en-us/windows-hardware/drivers/image/inf-files-for-still-image-devices

**Two Windows install topologies are possible:** an imaging
function with its own Plug and Play device object/INF, or
the documented multifunction-device approach that exposes
an Image-class interface through a single printer PDO and
a `WIASection`. The latter has distinct rules and cannot
be inferred solely from the existence of a second INF.
Investigate the actual PnP IDs and INF linkage before
selecting a topology.

**Critical protocol boundary:** The Windows text
`SubClass=StillImage` is **not** USB
`bInterfaceClass=0x06`, not a USB endpoint, and not proof
that observed vendor-defined MFC-290C interface
`0x01` (`FF/FF/FF`) implements scanning. The
existing `04f9:01fd` and scanner `MI_01` evidence
remain **candidates**, not raw descriptor captures.
This INF also cannot distinguish the shared-CIS flatbed
and ADF command protocol, or establish a native macOS ICA
driver by itself.

**Next read-only inspection**, without executing any
Windows driver or opening the physical machine:

```sh
# Choose the actual extracted scanner INF path.
grep -nEi '^\[|Class|SubClass|DeviceType|04F9|01FD|MI_01|MFC[-_ ]?290C|sti\.inf|Needs|WIASection|DeviceData|TwainDS|CopyFiles|USDClass|Capabilities|Events' path/to/scanner.inf
```

Follow the device's `[Models]` entry to the selected
`[DDInstall]` and `.Services` sections. Compare
referenced binary filenames and versions with the
printer INF and the already established USB/device
and CIS/ADF hardware ledger. Do not copy proprietary
binaries into the public repository or invent scanner
USB requests based on INF keywords.

Authoritative references:
- Microsoft, WIA INF entries, `DeviceType` and
  `sti.inf` USB sections:
  https://learn.microsoft.com/en-us/windows-hardware/drivers/image/inf-files-for-wia-devices
- Microsoft, legacy still-image INF installer:
  https://learn.microsoft.com/en-us/windows-hardware/drivers/image/inf-files-for-still-image-devices
- Microsoft, multifunction scanning via single PDO:
  https://learn.microsoft.com/en-us/windows-hardware/drivers/image/installing-scanning-functionality-in-an-mfp-with-a-single-pdo

**Do not confuse extensions:** Windows Brother `.PDD` is
**not automatically** a CUPS/PostScript `.PPD` (PostScript Printer
Description). Brother's official method for producing a PPD from
a Windows download requires a compatible **BR-Script/PostScript**
driver and a `.pp_` source. No evidence establishes that
this inkjet model's `.PDD` is plain text, Adobe PPD,
PCL metadata, a standard file format or an executable.
Classify its structure only from a real bounded byte inspection.

**Safe, read-only local workflow, macOS shell; run in the
extracted driver directory:**

```sh
find . -type f \( -iname '*.inf' -o -iname '*.pdd' -o -iname '*.ini' \) -print
# After choosing the actual INF path:
grep -nEi 'FeatureScore|MFC[-_ ]?290C|VID_04F9|PID_01FD|BRMF' path/to/driver.inf
# After choosing the actual PDD file path:
file path/to/confirmed.pdd
wc -c < path/to/confirmed.pdd
shasum -a 256 path/to/confirmed.pdd
hexdump -C -n 128 path/to/confirmed.pdd
```

The first-pass inventory must preserve **original bytes**; do
not rename `.PDD` to `.PPD` or run/install/execute any
untrusted or legacy driver. Where `BRMF290C.PDD` is
available, repeat the same bounded inspection, compare
sizes/hashes and inspect **differences in their contents
only after** verifying their packaging/version provenance.
Search `.INF` and `.INI` references without overwriting
the extracted files, and use PE import/export inspection of
known driver DLLs only if the static resource evidence
warrants it. No raw proprietary binary is committed to
this public repository.

**Potential information gates:**

| Question | Minimum required evidence | Driver implication |
| --- | --- | --- |
| Is the earlier `BRMF250C.PDD` filename accurate? | Corrected user-provided INF text and directory inventory | Previous filename is **not confirmed**; no family resource alias inferred |
| Is a `BRMF290C.PDD` present? | Verified directory listing and hashes of both files | Prefer the model-specific resource when mapping options |
| What is the PDD format? | Size, first bytes/magic, string and record layout from actual file | Write a **read-only offline parser** only for a validated format |
| Which features are recorded? | Authenticated records cross-checked with Brother manuals | May yield paper sizes, print modes, DPI, inks, media and constraints |
| Does PDD encode USB or raster data? | Cross-reference to the producing driver DLL and verified output format/capture | **Not inferred** from filename, extension or feature strings |
| Can macOS reuse it? | Independently understood, lawful data semantics and a working native print filter | **Metadata may be portable; Windows binary/code is not** |

**macOS design boundary:** Keep Brother print raster/transport analysis
distinct from CUPS/IPP queue metadata and from ICA scanning.
The Windows `.PDD` cannot itself make the MFC-290C AirPrint-compatible,
reveal scanner/PC-FAX USB opcodes, or replace a native macOS encoder.
Use the original MFC-290C CUPS/Linux printer-driver source as an
independent comparison point where licensing and actual source contents
permit. The existing `04f9:01fd` USB identity, unknown vendor
interface roles, fax safety rules and read-only host probes are unchanged.

Sources:
- Brother MFC-290C official Windows driver download catalog:
  https://support.brother.com/g/b/downloadlist.aspx?c=us&lang=en&os=6&prod=mfc290c_all
- Brother official Linux downloads, including CUPS wrapper source:
  https://support.brother.com/g/b/downloadlist.aspx?c=us_ot&lang=en&os=128&prod=mfc290c_all
- Third-party Brother DCP-585CW package inventory (shares
  the family file list; **not a verified MFC-290C install mapping**):
  https://www.runonpc.com/driverdownload/brother-dcp-585cw-printer-driver-download/
- Brother official PPD extraction instructions for **BR-Script**
  models, showing why `.PDD` and `.PPD` must not be conflated:
  https://help.brother-usa.com/app/answers/detail/a_id/164936

## USB command recovery: standards, interfaces, and unknowns (2026-11-21)

This is an **evidence inventory**, not a command sender. Neither Brother's
manuals, the existing macOS IORegistry output, nor our synthetic tests provide
an authenticated **raw** MFC-290C USB configuration/endpoint descriptor
or device-side command transcript yet. The reported interface class triplets
for interfaces 0 and 1 are observations, not raw descriptor captures. The
model identity `04f9:01fd` and host location ID do not establish actual
endpoint addresses, firmware revision or fax command bytes.

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

### Rear LINE and EXT. telephone jacks (physical observation, 2026-11-21)

**Observed on the user's unit:** Two separately marked analog telephone
modular jacks: `LINE` and `EXT.`. The rear labels establish two
**physical connections**, not two independent telephone subscriptions
and not a USB/PC serial-modem port.

Brother's original MFC-290C Quick Setup Guide, step 3, explicitly
requires the wall telephone cord to enter `LINE`. A separate
**external telephone** or external **telephone answering device
(TAD)** may connect to `EXT.` after removing its protective cap.
For an answering device Brother instructs selection of the
`External TAD` receive mode. An **extension telephone** at another
wall socket on the same line is distinct from an **external
telephone** connected to `EXT.`; both are recognized in the
MFC-290C User's Guide, chapter 7. This is line sharing, not
evidence for two concurrent independent modem channels or a
transparent, always-closed electrical pass-through.

```text
Analog wall line --> [LINE] Brother MFC-290C [EXT.] --> external phone
                                                   or answering device
USB host computer --> [USB device connector]   (separate interface)
```

**Verified behavior:** Under the documented answering-device
arrangement, the external TAD answers voice calls and the Brother
machine can listen for fax-sending tones and take over reception.
Brother also documents Fax/Tel receive behavior, external-phone
status and user actions via the control panel. These are
**standalone analog-line firmware behaviors**, not PC-FAX reception
and not evidence of an exposed USB CDC ACM, Hayes AT command or
user-space OS serial port.

**Hardware candidate triangulation:** Trace the two physical
jacks into the MFC-290C's **MJ PCB family `B53K967`**
(US/CAN assembly candidate `LT0405001`) and the
service-manual **MODEM/SDAA** blocks. Candidate functions
to check in the **correct MFC-290C MJ schematic** include
telephone-line surge/protection, ringing/call detection,
on-hook/off-hook behavior, line access and isolation/steering
for the external jack. A **relay, hook-switch chip,
DAA manufacturer, switching topology, exact passive
network and connection pinout remain unknown**: do not
promote generic fax designs or parts from related models
into verified MFC-290C components. The existence of the
jacks establishes use cases, not those specific implementations.

**Next non-invasive verification:** Compare the US/CAN
MFC-290C `LINE`/`EXT.` harnesses and actual
`B53K967` assembly with SM-FAX098 Appendix 5/6
(MJ PCB wiring/circuit scope) and the regional parts
list. Capture official part designators and datasheet
mechanical footprints **before** excluding candidates by
physical width. No live telephone-line test, wiring
modification, plug-in modem probe, dialing, ringing-voltage
measurement or speculative vendor USB command is necessary.

Sources:
- Brother MFC-290C original US Quick Setup Guide, step 3:
  https://download.brother.com/welcome/doc002385/cv_mfc290_usaeng_qsg.pdf
- Brother MFC-290C US User's Guide, chapter 7:
  https://www.bhphotovideo.com/lit_files/142624.pdf
- Brother MFC-290C FAQ on external TAD and Fax/Tel:
  https://support.brother.com/g/b/faqend.aspx?c=us&faqid=faq00002454_000&lang=en&pfs=1&prod=mfc290c_all
  https://support.brother.com/g/b/faqend.aspx?c=us_ot&faqid=faq00002455_000&lang=en&prod=mfc290c_all
- Brother June 2008 SM-FAX098, ch. 1, §3.1, App. 5/6:
  https://manualmachine.com/brother/dcp145c/18240960-service-manual/


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

The host-neutral files `drivers/usb/brother/mfc290c_fax.{h,c}` provide
strict **documented-versus-unsupported** classifications and a small PC-FAX
send-document preflight. The preflight accepts a nonempty monochrome page
count and rejects empty, color or unknown image modes. It does **not**
validate telephone numbers, raster encodings, call setup, live device state,
memory availability, fax negotiation, actual document contents, or a job's
ability to transmit. Its `DOCUMENT_ACCEPTABLE` result describes *only*
these static constraints. A documented fax mode returns
`WHP_MFC290C_FAX_DOCUMENTED_ONLY`, never "driver ready."

The model-specific fax test (`tests/brother-mfc290c-fax.c`) checks all six
separate modes, unsupported PC receive and color PC send, invalid inputs,
the existing `mfc290c.h` capability mask, and the distinction between
a memory *rating* and a hard job-size limit. The existing
`./build test-brother-mfc290c` target runs these tests on supported host
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
