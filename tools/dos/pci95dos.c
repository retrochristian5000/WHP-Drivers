/* WHPPCI.EXE: read-only real-mode DOS PCI BIOS diagnostics.
 * A DOS utility, NOT a TSR, chipset initializer, PCI.VXD or NDIS driver.
 * No direct CF8/CFC PCI access, MMIO, bus-master enabling or config writes.
 */
#ifndef __WATCOMC__
#error "Build this DOS entry point with OpenWatcom 16-bit wcc"
#endif
#include <i86.h>
#include <stdio.h>
#include <string.h>
#include "pci_bios_read.h"
#include "pci_bridge.h"
#include "pci_dos_memory.h"
#include "amd12_topology.h"
#include "rtl8111e.h"
#include "dos_version.h"
#include "identity.h" /* storage mode evidence only */
#include "pci_hci.h" /* USB HCI evidence only */

static uint8_t config_bytes[WHP_PCI95_BIOS_CONFIG_SIZE];
static struct whp_pci95_bridge bridge_info;
static struct whp_amd12_pcie_port_info root_info;
static struct whp_rtl8111e_pci_snapshot nic_info;
static struct whp_storage_pci_identity storage_info;
static struct whp_usb_pci_hci usb_info;
static struct whp_pci_dos_bar pci_bar;

static int bios_word(void *ctx, uint8_t bus, uint8_t devfn,
                     uint8_t offset, uint16_t *value)
{
    union REGS inregs, outregs;
    (void)ctx;
    memset(&inregs, 0, sizeof(inregs));
    memset(&outregs, 0, sizeof(outregs));
    inregs.w.ax = 0xb109u; /* PCI BIOS read configuration WORD. */
    inregs.h.bh = bus;
    inregs.h.bl = devfn;
    inregs.w.di = offset;
    int86(0x1a, &inregs, &outregs);
    if (outregs.w.cflag || outregs.h.ah != 0u)
        return outregs.h.ah == 0x86u ? 0 : -1;
    *value = outregs.w.cx;
    return 1;
}

static int pci_bios_installed(unsigned int *last_bus,
                              unsigned int *version)
{
    union REGS inregs, outregs;
    memset(&inregs, 0, sizeof(inregs));
    memset(&outregs, 0, sizeof(outregs));
    inregs.w.ax = 0xb101u; /* PCI BIOS installation check. */
    int86(0x1a, &inregs, &outregs);
    /* 16-bit int86 gives DX's low half of the 32-bit 'PCI ' signature.
     * Verify only the accessible half (0x4350), status and BCD version.
     * This is an availability preflight, not an authentication method.
     */
    if (outregs.w.cflag || outregs.h.ah != 0u ||
        outregs.w.dx != 0x4350u || outregs.w.bx < 0x0200u)
        return 0;
    *last_bus = (unsigned int)outregs.h.cl;
    *version = (unsigned int)outregs.w.bx;
    return 1;
}

static void print_caps(const char *label, int status, unsigned int pcie)
{
    if (status == 1) {
        if (pcie)
            printf("    %s: PCIe capability at %02Xh (not OS support)\n", label, pcie);
        else
            printf("    %s: complete; no PCIe capability in standard list\n", label);
    } else if (status == 2) {
        printf("    %s: capability chain incomplete; unknown\n", label);
    } else if (status == 3) {
        printf("    %s: capability chain malformed; unknown\n", label);
    } else {
        printf("    %s: no standard capability list advertised\n", label);
    }
}

/* Version reporting is separate from PCI BIOS availability and must
 * never enable FAT32, long filename or protected-mode calls by itself.
 */
static void report_dos_version(void)
{
    union REGS inregs, outregs;
    struct whp_dos_version dos;
    unsigned int reported_major, reported_minor, true_major, true_minor;
    int true_valid;

    memset(&inregs, 0, sizeof(inregs));
    memset(&outregs, 0, sizeof(outregs));
    inregs.h.ah = 0x30u; /* Reported version; SETVER may override it. */
    int86(0x21, &inregs, &outregs);
    reported_major = outregs.h.al;
    reported_minor = outregs.h.ah;

    memset(&inregs, 0, sizeof(inregs));
    memset(&outregs, 0, sizeof(outregs));
    inregs.w.ax = 0x3306u; /* DOS 5+ true version where available. */
    int86(0x21, &inregs, &outregs);
    true_major = outregs.h.bl;
    true_minor = outregs.h.bh;
    true_valid = !outregs.w.cflag && outregs.h.al != 0xffu &&
                 true_major >= 5u && true_minor < 100u;

    whp_dos_version_classify(reported_major, reported_minor, true_valid,
                             true_major, true_minor, &dos);
    printf("WHPPCI: reported DOS %u.%02u", (unsigned int)dos.reported_major,
           (unsigned int)dos.reported_minor);
    if (dos.true_known)
        printf("; true DOS %u.%02u\n", (unsigned int)dos.true_major,
               (unsigned int)dos.true_minor);
    else
        puts("; true DOS version unavailable (reported value may be spoofed)");

    switch (dos.profile) {
    case WHP_DOS_PROFILE_622:
        puts("DOS 6.22 profile: use legacy 8.3/FAT12/FAT16 assumptions.");
        break;
    case WHP_DOS_PROFILE_700:
        puts("DOS 7.00 profile: do not assume native FAT32 support.");
        break;
    case WHP_DOS_PROFILE_710:
        puts("DOS 7.10 profile: FAT32 possible, LFN APIs still require probing.");
        break;
    default:
        puts("DOS API profile unknown/other: no extended APIs assumed.");
        break;
    }
    puts("PCI BIOS discovery is independent of DOS filesystem services.");
}


/* INT 12h reports a firmware conventional-memory boundary, not free RAM.
 * No DOS allocator, upper-memory manager, or physical memory mapping calls.
 */
static unsigned int bios_conventional_memory_kb(void)
{
    union REGS inregs, outregs;
    memset(&inregs, 0, sizeof(inregs));
    memset(&outregs, 0, sizeof(outregs));
    int86(0x12, &inregs, &outregs);
    if (outregs.w.ax < 64u || outregs.w.ax > 1024u)
        return 0u;
    return (unsigned int)outregs.w.ax;
}

static void print_memory_window(const char *kind, uint64_t base,
                                uint64_t limit, unsigned int conventional_kb,
                                int memory_decode_enabled)
{
    unsigned int flags = whp_pci_dos_window_flags(
        base, limit, conventional_kb, memory_decode_enabled);
    if (!(flags & WHP_PCI_DOS_WINDOW_PRESENT))
        return;
    printf("    %s MMIO window %08lX%08lX-%08lX%08lX; decode %s\\n",
           kind,
           (unsigned long)(uint32_t)(base >> 32),
           (unsigned long)(uint32_t)base,
           (unsigned long)(uint32_t)(limit >> 32),
           (unsigned long)(uint32_t)limit,
           (flags & WHP_PCI_DOS_WINDOW_DECODE_ENABLED) ? "enabled" : "off");
    if ((flags & WHP_PCI_DOS_WINDOW_DECODE_ENABLED) &&
        (flags & WHP_PCI_DOS_WINDOW_TOUCHES_CONVENTIONAL))
        puts("    WARNING: decoded window overlaps BIOS conventional-address range.");
    else if (flags & WHP_PCI_DOS_WINDOW_TOUCHES_LOW_1M)
        puts("    NOTE: window includes some low 1-MiB physical addresses.");
    if (flags & WHP_PCI_DOS_WINDOW_SPANS_4G)
        puts("    NOTE: window straddles 4 GiB; DOS cannot map it directly.");
    if (flags & WHP_PCI_DOS_WINDOW_ABOVE_4G)
        puts("    NOTE: window begins above 4 GiB; DOS cannot map it directly.");
}

static void print_bar_memory(const uint8_t *cfg, size_t cfg_size,
                             unsigned int conventional_kb)
{
    unsigned int index, max_bars;
    unsigned int command = (unsigned int)cfg[4] |
                           ((unsigned int)cfg[5] << 8);
    max_bars = (cfg[0x0e] & 0x7fu) == 1u ? 2u : 6u;
    for (index = 0u; index < max_bars; ++index) {
        int status = whp_pci_dos_read_bar(cfg, cfg_size, index, &pci_bar);
        enum whp_pci_dos_bar_region region;
        if (status < 0) {
            printf("    BAR%u: invalid/unsupported descriptor; not mapped\\n", index);
            continue;
        }
        if (status == 0)
            continue;
        region = whp_pci_dos_bar_region_of(&pci_bar, conventional_kb);
        printf("    BAR%u %s%s base=%08lX%08lX; decode %s\\n",
               index, pci_bar.is_io ? "I/O" : "MMIO",
               pci_bar.is_64 ? " 64-bit" : "",
               (unsigned long)(uint32_t)(pci_bar.base >> 32),
               (unsigned long)(uint32_t)pci_bar.base,
               (command & (pci_bar.is_io ? 1u : 2u)) ? "enabled" : "off");
        switch (region) {
        case WHP_PCI_DOS_BAR_IO_OUTSIDE_16BIT:
            puts("    NOTE: I/O BAR base is outside x86 16-bit port space.");
            break;
        case WHP_PCI_DOS_BAR_CONVENTIONAL_BASE:
            puts("    WARNING: MMIO BAR base lies within reported conventional RAM.");
            break;
        case WHP_PCI_DOS_BAR_LEGACY_BASE:
            puts("    NOTE: MMIO BAR base lies below 1 MiB (legacy physical region).");
            break;
        case WHP_PCI_DOS_BAR_PHYSICAL_MAPPING_REQUIRED:
            puts("    NOTE: MMIO base requires a verified physical-memory mapping.");
            break;
        case WHP_PCI_DOS_BAR_ABOVE_4G:
            puts("    NOTE: MMIO base above 4 GiB is not DOS directly mappable.");
            break;
        default:
            break; /* In-range I/O port address is not a safe-use claim. */
        }
        if (pci_bar.slots == 2u)
            ++index; /* Upper half belongs to this 64-bit BAR. */
    }
}

int main(void)
{
    struct whp_pci95_bios_reader reader;
    enum whp_pci95_bios_result result;
    unsigned int last_bus, version, bus, dev, fn, multifn;
    unsigned int found_bridges = 0, found_nics = 0, found_hda = 0, found_storage = 0, found_usb = 0, read_errors = 0;
    unsigned int snapshot_size, devfn;
    unsigned int id_vendor, id_device, cls, subclass;
    int wanted_bridge, wanted_nic, wanted_hda, wanted_storage, wanted_usb;
    unsigned int conventional_kb;

    report_dos_version();
    conventional_kb = bios_conventional_memory_kb();
    if (conventional_kb)
        printf("WHPPCI: BIOS conventional-memory boundary=%u KiB (not free RAM)\\n",
               conventional_kb);
    else
        puts("WHPPCI: BIOS conventional-memory boundary unavailable.");
    if (!pci_bios_installed(&last_bus, &version)) {
        puts("WHPPCI: no usable INT 1Ah PCI BIOS (B101h).");
        puts("No hardware probing was attempted. This does not prove PCIe is absent.");
        return 2;
    }
    printf("WHPPCI: PCI BIOS version %02Xh, last reported bus %u\n",
           version, last_bus);
    puts("READ ONLY. PCI BIOS configuration bytes 00h-FFh only.");
    reader.read_word = bios_word;
    reader.context = 0;
    for (bus = 0; bus <= last_bus; ++bus) {
        for (dev = 0; dev < 32u; ++dev) {
            multifn = 0;
            for (fn = 0; fn < 8u; ++fn) {
                if (fn != 0u && !multifn)
                    break;
                devfn = (dev << 3) | fn;
                result = whp_pci95_bios_read_config(&reader, (uint8_t)bus,
                           (uint8_t)devfn, config_bytes, 64u);
                if (result == WHP_PCI95_BIOS_NO_DEVICE) {
                    if (fn == 0u) break;
                    continue;
                }
                if (result != WHP_PCI95_BIOS_OK) {
                    printf("    PCI BIOS read failure at %u:%u.%u\n",
                           bus, dev, fn);
                    ++read_errors;
                    if (fn == 0u) break;
                    continue;
                }
                if (fn == 0u)
                    multifn = (unsigned int)((config_bytes[0x0eu] & 0x80u) != 0u);
                id_vendor = (unsigned int)(config_bytes[0] |
                           ((unsigned int)config_bytes[1] << 8));
                id_device = (unsigned int)(config_bytes[2] |
                           ((unsigned int)config_bytes[3] << 8));
                cls = config_bytes[0x0bu];
                subclass = config_bytes[0x0au];
                wanted_bridge = cls == 0x06u && subclass == 0x04u &&
                          (config_bytes[0x0eu] & 0x7fu) == 1u;
                wanted_nic = id_vendor == 0x10ecu && id_device == 0x8168u &&
                          cls == 0x02u && subclass == 0u;
                /* HDA is PCI class 04:03 / PI=00; this identifies only
                 * a controller candidate, never a codec or DMA interface.
                 */
                wanted_hda = cls == 0x04u && subclass == 0x03u &&
                             config_bytes[9] == 0u &&
                             (config_bytes[0x0eu] & 0x7fu) == 0u;
                /* PCI mass-storage: IDE, RAID or SATA controller class.
                 * This does not prove that any RAID volume exists.
                 */
                wanted_storage = cls == 0x01u &&
                      (subclass == 0x01u || subclass == 0x04u ||
                       subclass == 0x06u);
                /* 0C:03 is USB; PI identifies UHCI/OHCI/EHCI/xHCI.
                 * FEh is a USB device, not a host controller.
                 */
                wanted_usb = cls == 0x0cu && subclass == 0x03u &&
                             config_bytes[9] != 0xfeu &&
                             (config_bytes[0x0eu] & 0x7fu) == 0u;
                if (!wanted_bridge && !wanted_nic && !wanted_hda &&
                    !wanted_storage && !wanted_usb) {
                    if (cls == 0x06u && subclass == 0u)
                        printf("%u:%02u.%u host bridge %04X:%04X (not PCI type-1)\n",
                               bus, dev, fn, id_vendor, id_device);
                    continue;
                }
                /* A full 256-byte snapshot is optional for capabilities.
                 * Failure does not invalidate the already-read 64-byte header.
                 */
                snapshot_size = 64u;
                if (whp_pci95_bios_read_config(&reader, (uint8_t)bus,
                         (uint8_t)devfn, config_bytes, 256u) == WHP_PCI95_BIOS_OK)
                    snapshot_size = 256u;
                else {
                    result = whp_pci95_bios_read_config(&reader, (uint8_t)bus,
                                   (uint8_t)devfn, config_bytes, 64u);
                    if (result != WHP_PCI95_BIOS_OK) {
                        ++read_errors;
                        continue; /* Device was not stable during read. */
                    }
                }
                if (wanted_usb &&
                    whp_usb_pci_hci_decode(config_bytes, snapshot_size,
                                           &usb_info)) {
                    ++found_usb;
                    printf("%u:%02u.%u USB %04X:%04X class=0C:03:%02X HCI=%s\n",
                           bus, dev, fn,
                           (unsigned int)usb_info.vendor,
                           (unsigned int)usb_info.device,
                           (unsigned int)usb_info.prog_if,
                           whp_usb_hci_name(usb_info.hci));
                    printf("    PCI-rev=%02Xh subsystem=%04X:%04X command=%04Xh IRQ=%u/%u\n",
                           (unsigned int)usb_info.revision,
                           (unsigned int)usb_info.subsystem_vendor,
                           (unsigned int)usb_info.subsystem_device,
                           (unsigned int)usb_info.command,
                           (unsigned int)usb_info.irq_line,
                           (unsigned int)usb_info.irq_pin);
                    printf("    %s\n", whp_usb_hci_speed_context(usb_info.hci));
                    if (usb_info.amd_family_id_hint != WHP_USB_HCI_UNKNOWN)
                        printf("    AMD FCH family ID hints %s%s\n",
                               whp_usb_hci_name(usb_info.amd_family_id_hint),
                               usb_info.id_pi_conflict ?
                               "; PCI PI disagrees: investigate" : "");
                    puts("    Physical ports, devices, routing and active speeds NOT verified.");
                }
                if (wanted_storage &&
                    whp_storage_decode_pci(config_bytes, snapshot_size,
                                           &storage_info)) {
                    ++found_storage;
                    printf("%u:%02u.%u STORAGE %04X:%04X class=01:%02X:%02X mode=%s\n",
                           bus, dev, fn,
                           (unsigned int)storage_info.vendor,
                           (unsigned int)storage_info.device,
                           (unsigned int)storage_info.subclass,
                           (unsigned int)storage_info.prog_if,
                           whp_storage_mode_name(storage_info.pci_mode));
                    printf("    PCI-rev=%02Xh subsystem=%04X:%04X command=%04Xh IRQ=%u/%u\n",
                           (unsigned int)storage_info.revision,
                           (unsigned int)storage_info.subsystem_vendor,
                           (unsigned int)storage_info.subsystem_device,
                           (unsigned int)storage_info.command,
                           (unsigned int)storage_info.irq_line,
                           (unsigned int)storage_info.irq_pin);
                    if (storage_info.amd_fch_id_hint != WHP_STORAGE_MODE_UNKNOWN) {
                        printf("    AMD FCH SATA ID hints %s%s\n",
                               whp_storage_mode_name(storage_info.amd_fch_id_hint),
                               storage_info.mode_disagrees ?
                               "; PCI class/PI differs: investigate" : "");
                    }
                    puts("    Firmware selection and RAID array state NOT verified.");
                }
                if (wanted_bridge &&
                    whp_pci95_decode_bridge(config_bytes, snapshot_size,
                                            &bridge_info) == WHP_PCI95_PARSE_OK) {
                    ++found_bridges;
                    printf("%u:%02u.%u BRIDGE %04X:%04X primary=%u child=%u last=%u\n",
                           bus, dev, fn, id_vendor, id_device,
                           (unsigned int)bridge_info.primary,
                           (unsigned int)bridge_info.secondary,
                           (unsigned int)bridge_info.subordinate);
                    printf("    PCI command %04Xh; memory window %s\n",
                           (unsigned int)bridge_info.command,
                           bridge_info.mem_window_present ? "declared" :
                                                            "disabled/invalid");
                    print_caps("bridge", (int)bridge_info.caps,
                               (unsigned int)bridge_info.pcie_cap_offset);
                    if (bridge_info.mem_window_present)
                        print_memory_window("nonprefetchable",
                                           (uint64_t)bridge_info.mem_base,
                                           (uint64_t)bridge_info.mem_limit,
                                           conventional_kb,
                                           (bridge_info.command & 2u) != 0u);
                    if (bridge_info.pref_window_present)
                        print_memory_window("prefetchable",
                                           bridge_info.pref_base,
                                           bridge_info.pref_limit,
                                           conventional_kb,
                                           (bridge_info.command & 2u) != 0u);
                    /* AMD BKDG Family 12h section 2.11: port location is
                     * an identity clue, not proof of a populated link.
                     * Never change indexed NB/PCIe configuration registers.
                     */
                    if (whp_amd12_pcie_decode(config_bytes, snapshot_size,
                              bus, dev, fn, bridge_info.pcie_cap_offset,
                              &root_info)) {
                        printf("    %s: %s (documented topology candidate)\n",
                               whp_amd12_pcie_origin_name(root_info.origin),
                               whp_amd12_pcie_port_name(root_info.port));
                        if (root_info.pcie_link_status_valid)
                            printf("    PCIe negotiated speed-code=%u width=x%u DLL-active-bit=%u\n",
                                   (unsigned int)root_info.negotiated_speed_code,
                                   (unsigned int)root_info.negotiated_width,
                                   (unsigned int)root_info.link_active);
                        else
                            puts("    PCIe Link Status not in validated snapshot.");
                        puts("    Link training, physical wiring and driver binding NOT verified.");
                    }
                }
                if (wanted_hda) {
                    unsigned int subsystem_vendor =
                        (unsigned int)(config_bytes[0x2cu] |
                        ((unsigned int)config_bytes[0x2du] << 8));
                    unsigned int subsystem_device =
                        (unsigned int)(config_bytes[0x2eu] |
                        ((unsigned int)config_bytes[0x2fu] << 8));
                    ++found_hda;
                    printf("%u:%02u.%u HDA controller candidate %04X:%04X\n",
                           bus, dev, fn, id_vendor, id_device);
                    printf("    PCI-rev=%02Xh subsystem=%04X:%04X%s\n",
                           (unsigned int)config_bytes[8],
                           subsystem_vendor, subsystem_device,
                           id_vendor == 0x1022u && id_device == 0x780du ?
                           " (AMD FCH Azalia family candidate)" : "");
                    puts("    Codec identity, HDA MMIO and sound playback NOT tested.");
                }
                if (wanted_nic &&
                    whp_rtl8111e_decode_pci_snapshot(config_bytes,
                              snapshot_size, &nic_info) == WHP_PCI_SNAPSHOT_OK) {
                    ++found_nics;
                    printf("%u:%02u.%u REALTEK FAMILY %04X:%04X PCI-rev=%02Xh\n",
                           bus, dev, fn, id_vendor, id_device,
                           (unsigned int)nic_info.pci_revision);
                    printf("    subsystem=%04X:%04X command=%04Xh IRQ-line=%u\n",
                           (unsigned int)nic_info.subsystem_vendor,
                           (unsigned int)nic_info.subsystem_device,
                           (unsigned int)nic_info.command,
                           (unsigned int)nic_info.irq_line);
                    print_caps("endpoint", (int)nic_info.capabilities,
                               (unsigned int)nic_info.pcie_cap_offset);
                    puts("    NOTE: PCI revision is NOT RTL8111E MAC TxConfig/XID.");
                }
                /* Inspect assigned BAR bases from the already-read PCI
                 * header. Sizes and ownership are unknown: no sizing writes,
                 * BIOS reconfiguration, physical dereferences, or DMA.
                 */
                print_bar_memory(config_bytes, snapshot_size, conventional_kb);
            }
        }
    }
    printf("WHPPCI: %u bridge(s), %u RTL8168/8111 candidate(s), %u HDA candidate(s), %u storage controller(s), %u USB HCI(s), %u read error(s).\n",
           found_bridges, found_nics, found_hda, found_storage, found_usb, read_errors);
    puts("A DOS PCI BIOS result is not Windows 95 PCI.VXD/NDIS validation.");
    return read_errors ? 1 : 0;
}
