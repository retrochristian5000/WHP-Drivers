# WHP-Drivers — RTL8111E Ethernet adapter, INF and miniport development

This document preserves detailed research moved from the root README. Commands and relative source paths in code blocks are relative to the **repository root**. See [the project overview](../README.md) for the shortest supported entry points.

## Realtek RTL8111E (HP Pavilion p6-2310; staged miniport development)

### RTL8111E Windows 95 RTM network INF research gate (2026-10-23)

`packaging/win95rtm/RTL8111E.INF` now exists as an **intentionally
non-installable** Net-class research scaffold. Its active metadata is
limited to `[Version]` (`Signature="$CHICAGO$"`, `Class=Net`,
provider), `[Manufacturer]`, an **empty** `[WHP.RTL8111E]` model
section, and `[Strings]`. The PCI candidate
`PCI\\VEN_10EC&DEV_8168` appears **only in comments**: this is a
family-wide RTL8168/RTL8111 ID, not a proven RTL8111E revision nor a
verified Windows 95 RTM PCI enumeration. The file has **no active
hardware binding, AddReg, CopyFiles, install section or executable
reference**. Do not copy it into `WINDOWS\\INF` or use Add Hardware
with it.

Period Windows 95 network INF evidence identifies the future activation
pieces: PCI device/model match, `.ndi` install section,
`DevLoader=*ndis`, `DeviceVxDs` referencing a real miniport `.SYS`
(not a renamed full-MAC `.VXD`), `Ndi\\Interfaces` with
`UpperRange=ndis3` / `LowerRange=ethernet`, `Ndi\\Install`
registration, source/destination file tables, and actual copied-driver
provenance. Those details are **checklist comments only**, not copied
guesses from a modern Windows NT `AddService` INF. A 1990s 3Com
`w95el90x.inf` is an informative historical witness, but its own
dual NDIS 2/3/4 settings and chip IDs are **not** this driver's ABI.

Run the narrow or combined checks (Python 3 only):

```sh
./build rtl8111e-inf-check
./build inf-check                 # Radeon + RTL8111E research scaffolds
WHP_DRIVER_PROFILE=rtl8111e ./build.sh test-fast
```

`tools/verify_rtl8111e_inf.py` rejects any new active key/section,
model mapping, copied binary, NT service stanza, malformed encoding,
removed safety notice, or changed documented PCI candidate.
`tests/test_rtl8111e_inf.py` adds 13 fail-closed regression cases;
the RTL8111E smoke target now runs the INF gate before compiling.
The existing Radeon INF tests remain unchanged. No driver binary is
built or installed as a consequence of this file or these commands.

To activate a real NIC package later, first obtain the exact target
PCI revision/subsystem, confirm Windows 95 RTM sees the PCI bridge,
finish the NDIS 3.1 adapter and DMA/cache/interrupt behavior, compile
and link against authenticated NT 3.51 DDK headers and import libs,
verify the actual driver image/loader in a disposable RTM guest, then
derive an installable INF from inspected period NET*.INF examples.

Evidence:
- Windows 95 DDK network differences and CardType / INF instructions:
  https://techshelps.github.io/MSDN/WIN95DDK/HTML/S2D16.HTM
- Historical 3Com Windows 95 network INF (comparison, not template authority):
  https://files.mpoli.fi/unpacked/hardware/net/3com/3c90xx2.exe/w95el90x.inf
- Microsoft NDI binding direction (modern description; cross-check only):
  https://learn.microsoft.com/en-us/windows-hardware/drivers/network/specifying-binding-interfaces



The HP Pavilion p6-2310 (H3Y76AA) has a reported Realtek RTL8111E
PCI Express Gigabit Ethernet controller. Prior WHP LISTPCI observations
found the controller at PCI bus 003, device 00, while Windows 95 RTM
Device Manager did not expose it. Windows 95 RTM `PCI.VXD` version
4.00.950, the exact PCI device/function/revision/subsystem IDs and the
PCIe bridge enumeration path remain separate verification gates.

The RTL8168/RTL8111 family shares PCI vendor/device ID `10EC:8168`;
that ID is **not** enough to distinguish RTL8111E from other revisions.
`drivers/network/rtl8111e/` now contains a portable data-only core
which checks Ethernet-class PCI identity (`02:00`), classifies externally
supplied `TxConfig` (register offset `0x40`) signature snapshots into
E1/E2/EVL candidates, and validates externally supplied MAC-address bytes.
Revision masks follow the Linux `r8169` chip table; unsupported signatures
remain *family candidates*, not asserted RTL8111E matches. No source code
in this module enumerates PCI, maps BARs, reads MMIO, or touches hardware.

Run `./build smoke-rtl8111e` for pinned LLVM i386 COFF compilation
and native-host regression tests (provided pinned toolchain is built).
An independent host Clang check also compiled the same source into
i386 COFF and passed the test vectors. This is **not yet an installable
Windows 95 driver**: there is no NDIS 3.1 miniport registration, LE
binary, hardware-connected DMA, IRQ handling, PHY management, transmit/receive path,
or active binding INF. A deliberately non-installable INF research scaffold
now exists; see its separately enforced safety gate above. Nothing has
been loaded on the Pavilion.

Keep the existing `./build ndis31-ddk-check` historical NT 3.51 DDK
gate. A loadable driver also requires read-only PCI inventory and a
verified Windows 95 RTM bus-enumeration path. Do not substitute NDIS 2
DOS, NDIS 5, an NT `.SYS`, or later Win9x headers without an explicit
compatibility analysis.

### RTL8111E read-only PCI configuration snapshot gate (2026-11-18)

The RTL8111E identity core now provides
`whp_rtl8111e_decode_pci_snapshot()`, a hardware-free decoder for a
caller-supplied **raw little-endian** PCI configuration-space byte dump.
It needs at least the 64-byte type-0 endpoint header; up to the first
256 bytes are relevant when interpreting standard PCI capabilities.
It records vendor/device ID, PCI revision (not chip XID), class and
programming interface, multifunction bit, PCI command/status, raw six
BAR slots, subsystem IDs, and legacy interrupt pin/line. BAR slots are
**not** decoded as validated physical mappings or written/probed for size.
PCI command bus-master/memory-decode flags are observed, not enabled.
A type-1 PCI bridge is rejected as a NIC endpoint, not misread as a
type-0 subsystem/BAR layout.

A bounded, cycle-checking parser follows the standard capability-list
pointers and reports PCIe, PM, MSI and MSI-X offsets only after the
entire chain is available and structurally valid. A 64-byte-only
snapshot with an advertised capability list is reported as **truncated**,
not falsely interpreted as lacking PCIe. Missing devices, malformed
pointers, cycles and unsupported header types have explicit outcomes.
These offsets indicate advertised capability structures, **not**
working legacy Win95 interrupt routing, MSI support or an authenticated
RTL8111E revision. Existing `TxConfig` E1/E2/EVL classification remains
separate because PCI revision IDs are not Realtek MAC XID signatures.

```sh
./build test-rtl8111e-pci-snapshot
WHP_DRIVER_PROFILE=rtl8111e ./build.sh test-fast
```

The new host-only test target compiles the pure decoder with a native
C compiler and exercises fabricated valid, truncated and malformed
configuration snapshots. It never bootstraps LLVM or accesses PCI.
`smoke-rtl8111e` additionally includes it after i386 COFF identity
compilation. Supply an actual configuration snapshot from an
independently trusted read-only inventory source **before** deriving
machine-specific claims. This does not add a PCI enumeration interface,
loadable NDIS miniport, firmware access, interrupt registration or DMA.

PCI header offset and capability definitions:
https://github.com/torvalds/linux/blob/master/include/uapi/linux/pci_regs.h

### RTL8111E DMA descriptor and ring core (2026-10-23)

The project now has an actual descriptor/ring **software implementation** in
`drivers/network/rtl8111e/dma_ring.h` and `dma_ring.c`, plus host-side
simulation vectors `tests/rtl8111e-dma-ring.c`. Unlike the earlier
identification and lifecycle-only code, this module writes and consumes
RTL8168-family **16-byte** TX/RX descriptors in little-endian byte layout:
`opts1`, `opts2`, address low and address high. It controls OWN
(bit 31), ring-end/EOR (bit 30), first-fragment/FS (bit 29), and
last-fragment/LS (bit 28). It checks the RX error summary, rejects
fragmented frames, and subtracts the four FCS bytes from valid RX
lengths. Offloads, jumbo frames and chained fragments are intentionally
not enabled.

**Boundaries and safeguards:**
- Only caller-supplied **already-mapped bus addresses**, never CPU virtual
  pointers converted into physical addresses. The first implementation
  conservatively requires a 32-bit address range, aligned 256-byte ring
  base, and eight 16-byte descriptors per ring. These are project policy
  limits, **not** claims about the RTL8111E's maximum DMA capabilities.
- Bounded 60–2048-byte TX frames and 64–2048-byte RX buffers; rejects
  address overflow, zero/oversized lengths, queues at capacity, and
  rebinding live rings. Buffer addresses cannot cross the 4 GiB ceiling.
- A six-function DMA **platform hook contract** supplies data synchronization,
  descriptor preparation **before OWN**, publication **after OWN**,
  completion acquisition, read barriers after cleared OWN, and RX data
  synchronization. It MUST be implemented by authenticated historical
  NDIS/OS services. A compiler fence or no-op hook is insufficient.
- Hardware completion does **not** automatically retire NDIS ownership.
  `whp_dma_complete` returns the descriptor result and leaves the
  buffer **CPU-held**; only `whp_dma_ack` after NDIS TX completion or
  RX payload copying/drop can retire its lifecycle ticket. Reset/halt
  remains blocked until both DMA rings and the lifecycle core are drained.
- Rejects suspicious device-modified buffer addresses or EOR bits and
  never releases DMA memory while ownership is unsettled. Synchronization
  calls must be serialized by the platform adapter; the ring is not
  inherently thread-safe.
- `whp_dma_unbind` only permits a quiesced HALTING/HALTED/RESETTING
  state and zero outstanding tickets. The verified platform must first
  disable DMA and interrupts, unmap buffers and release actual physical
  allocations; this core **does not** perform those privileged actions.

Run `./build smoke-rtl8111e` to compile `dma_ring.c` as an i386
Windows COFF object and exercise its simulator against the real portable
miniport lifecycle core. The source was also independently compiled
to i386 COFF and the DMA vectors passed locally under AddressSanitizer
and UBSan using a lightweight lifecycle mock. Those local tests are NOT
authentic NDIS driver tests.

The RTL8111E NDIS 3.1 adapter still returns failure on hardware
initialization, because the **NT 3.51 DDK DMA allocation and cache
synchronization APIs, actual PCI BAR/bus-master ownership, DMA masks,
interrupt masking and physical driver image have not been verified**.
Do not set the NIC's TX/RX ring registers or bus-master command bit
based solely on these simulated descriptors.

Hardware layout/reference witness (Linux r8169):
https://kernel.googlesource.com/pub/scm/linux/kernel/git/torvalds/linux/+/906dde0f355bd97c080c215811ae7db1137c4af8/drivers/net/ethernet/realtek/r8169.c

NDIS shared memory context (modern documentation, NOT NT 3.51 ABI
verification):
https://learn.microsoft.com/en-us/windows-hardware/drivers/network/using-shared-memory-in-network-drivers

### RTL8111E miniport lifecycle core (2026-10-23)

`miniport_core.h` and `miniport_core.c` provide **hardware-independent**
miniport lifecycle policy, not a loadable NDIS driver or a replacement DDK.
The core implements HALTED → INITIALIZING → RUNNING and explicit
RESETTING/HALTING drain paths. Initialization rejects unknown family
signatures and unusable MAC addresses. TX and RX have independently bounded
8-ticket bookkeeping pools with monotonically increasing serials; stale,
duplicate, out-of-range and wrong-direction completions are rejected.
Exhausting the serial space fails closed rather than reusing a ticket.

After reset/halt starts, new TX/RX work is refused. The drain operation
must complete/fail its **actual NDIS requests and receive indications**
before calling the corresponding ticket-retirement functions, and both
pools must be empty before reset/halt finishes. Failure to drain returns
`WHP_MP_BUSY` without discarding ownership. A failed reset returns to
HALTED; it does not silently resume network traffic.

**Concurrency boundary:** this core has no lock and is not thread-safe.
The eventual NT 3.51/Win95 NDIS miniport adapter MUST serialize all
calls with verified historical synchronization, and MUST own hardware
memory barriers, interrupt masking, DMA synchronization, NDIS completion
semantics and timeouts. The internal ticket cap is a software testing
limit, **not** a claimed RTL8111E DMA ring size. Do not invoke the core
from concurrently executing interrupt/send/reset code without that lock.
The PCIe/PnP enumeration and historical NDIS headers remain unverified.

Run `./build smoke-rtl8111e` to compile the identification *and* miniport
policy sources as i386 COFF, run host-only identity tests, and run the
miniport lifecycle regression. `./build.sh menuconfig` also offers
`rtl8111e`, and the `all` profile now includes its smoke check.
No DMA, IRQ, PHY, real packet path, `.VXD`, INF binding or RTL8111E
hardware access has been enabled.

Evidence:
- Microsoft Windows 95 DDK network model:
  https://techshelps.github.io/MSDN/WIN95DDK/HTML/S2D14.HTM
- Linux `r8169_main.c` RTL8168E/8111E revision mask and TxConfig:
  https://linux.googlesource.com/linux/kernel/git/torvalds/linux/+/6c790212c588fddeb0d852f2790840753bb604b1/drivers/net/ethernet/realtek/r8169_main.c
