# Brother MFC-290C — USB multifunction driver evidence and boundaries

**Status:** identity decoder and synthetic host tests only. **Not an installable driver.**

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
- USB transport, print stream encoding, scanner commands, PC-FAX sending, OS install packages, and real-hardware behavior remain **unimplemented**. No firmware updates, ink movement, phone calls, or hardware access occur in the identity test.

## Portability gates

| Target | Required proof before claiming driver support |
| --- | --- |
| DOS 6.x | USB host-controller access, enumeration and endpoint transport; a PCI inventory alone is insufficient. |
| Windows 95 RTM | A compatible USB host stack (RTM has no native USB support), plus the appropriate model/function driver ABI. Do not relabel a Windows 98 driver as RTM-compatible. |
| Windows 95 OSR2.x / Windows 98 | Verify exact release's USB stack and printer/scanner/fax function interfaces; do not silently borrow NT/WDM components. |
| Linux / BSD | Enumerate actual USB interfaces; integrate printing, scanning and PC-FAX with applicable user-space APIs only after protocol verification. Existing Brother Linux packages are separate prior art. |
| Modern macOS / Windows | Use supported, architecture-correct driver and USB APIs; confirm host OS restrictions and actual function protocols. |

## Verification

Run `./build.sh test-brother-mfc290c` (or `./build test-brother-mfc290c`). This uses a native C11 host compiler and synthetic descriptors; it does not need LLVM, OpenWatcom, USB permissions, or a printer. The targeted GitHub Actions job runs the same command when relevant code changes.

## Primary and corroborating references

- Brother, *Quick Setup Guide*, 2008 (USB speed, cable, physical phone connections): https://download.brother.com/welcome/doc002385/cv_mfc290_usaeng_qsg.pdf
- Brother MFC-290C manuals (User's Guide and Software User's Guide): https://support.brother.com/g/b/manualtop.aspx?c=us&lang=en&prod=mfc290c_all
- Brother MFC-290C specifications: https://support.brother.com/g/b/spec.aspx?c=us&lang=en&prod=mfc290c_all
- Brother PC-FAX receive exclusion: https://support.brother.com/g/s/id/htmldoc/mfc/mfc-5895cw/gb/html/sug/chapter6_3.html
- Brother Linux downloads: https://support.brother.com/g/b/downloadtop.aspx?c=us&lang=en&prod=mfc290c_all
- Independent USB ID corroboration: https://www.hamrick.com/es/vuescan/brother-mfc-290c-controlador.html and https://askubuntu.com/questions/151201/
