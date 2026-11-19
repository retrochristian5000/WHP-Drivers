/* WHPPROBE.EXE: read-only DOS system inventory (PCI + BIOS legacy).
 * A DOS utility, NOT a TSR, ISA auto-detect driver, PCI.VXD or NDIS driver.
 * No direct CF8/CFC PCI access, MMIO, bus-master enabling or config writes.
 */
#ifndef __WATCOMC__
#error "Build this DOS entry point with OpenWatcom 16-bit wcc"
#endif
#include <i86.h>
#include <conio.h>
#include <io.h>
#include <stdio.h>
#include <string.h>
#include "pci_bios_read.h"
#include "pci_bridge.h"
#include "pci_dos_memory.h"
#include "memory_survey.h"
#include "pci_config_report.h"
#include "probe_menu.h"
#include "amd12_topology.h"
#include "rtl8111e.h"
#include "dos_version.h"
#include "legacy_bios.h"
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

/* INT 11h equipment bits and the BIOS Data Area at 0040:0000 are
 * read-only historical resources; neither discovers physical ISA slots.
 * Read only the first seven BDA words (COM1-4 and LPT1-3).
 */
static void probe_legacy_bios(void)
{
    union REGS inregs, outregs;
    struct whp_dos_legacy_snapshot snapshot;
    struct whp_dos_legacy_report report;
    const volatile unsigned short __far *bda;
    unsigned int i;

    memset(&inregs, 0, sizeof(inregs));
    memset(&outregs, 0, sizeof(outregs));
    memset(&snapshot, 0, sizeof(snapshot));
    int86(0x11, &inregs, &outregs);
    snapshot.equipment = (uint16_t)outregs.w.ax;
    bda = (const volatile unsigned short __far *)MK_FP(0x0040u, 0x0000u);
    for (i = 0u; i < WHP_DOS_BDA_COM_PORTS; ++i)
        snapshot.com_base[i] = (uint16_t)bda[i];
    for (i = 0u; i < WHP_DOS_BDA_LPT_PORTS; ++i)
        snapshot.lpt_base[i] = (uint16_t)bda[4u + i];

    if (!whp_dos_legacy_decode(&snapshot, &report)) {
        puts("Legacy BIOS equipment decoding unavailable.");
        return;
    }
    printf("WHPPROBE: BIOS legacy equipment word=%04Xh (not ISA-card detection)\n",
           (unsigned int)report.equipment);
    printf("    BIOS flags: serial=%u parallel=%u floppy=%u math=%u game=%u video-code=%u\n",
           report.bios_serial_count, report.bios_parallel_count,
           report.bios_floppy_count, report.bios_fpu_present,
           report.bios_game_port_reported, report.bios_video_mode_code);
    for (i = 0u; i < WHP_DOS_BDA_COM_PORTS; ++i)
        if (report.com_base[i] != 0u)
            printf("    BIOS COM%u base=%04Xh (I/O not accessed)\n",
                   i + 1u, (unsigned int)report.com_base[i]);
    for (i = 0u; i < WHP_DOS_BDA_LPT_PORTS; ++i)
        if (report.lpt_base[i] != 0u)
            printf("    BIOS LPT%u base=%04Xh (I/O not accessed)\n",
                   i + 1u, (unsigned int)report.lpt_base[i]);
    puts("    ISA PnP card identity, IRQ/DMA allocation and physical slots unverified.");
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
    printf("WHPPROBE: reported DOS %u.%02u", (unsigned int)dos.reported_major,
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
    printf("    %s MMIO window %08lX%08lX-%08lX%08lX; decode %s\n",
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
            printf("    BAR%u: invalid/unsupported descriptor; not mapped\n", index);
            continue;
        }
        if (status == 0)
            continue;
        region = whp_pci_dos_bar_region_of(&pci_bar, conventional_kb);
        printf("    BAR%u %s%s base=%08lX%08lX; decode %s\n",
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
            if (command & 2u)
                puts("    WARNING: decoded MMIO BAR base is inside reported conventional RAM.");
            else
                puts("    NOTE: BAR base in conventional range, but PCI memory decode is off.");
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

/* Named fields come only from a BIOS-read PCI configuration snapshot.
 * Never touch a BAR address, read device MMIO or infer active driver support.
 */
static void print_pci_registers(const uint8_t *cfg, unsigned int length)
{
    struct whp_pci_config_report r;
    unsigned int index;
    if (!whp_pci_config_report_decode(cfg, length, &r)) {
        puts("    PCI configuration register report unavailable.");
        return;
    }
    printf("    PCI registers 00h Vendor=%04Xh Device=%04Xh\n",
           (unsigned int)r.vendor, (unsigned int)r.device);
    printf("    PCI 04h Command=%04Xh Status=%04Xh; "
           "08h Revision=%02Xh PI=%02Xh Subclass=%02Xh Class=%02Xh\n",
           (unsigned int)r.command, (unsigned int)r.status,
           (unsigned int)r.revision, (unsigned int)r.prog_if,
           (unsigned int)r.subclass, (unsigned int)r.class_code);
    printf("    PCI 0Ch CacheLine=%02Xh Latency=%02Xh Header=%02Xh BIST=%02Xh; "
           "3Ch IRQ-line=%02Xh IRQ-pin=%02Xh\n",
           (unsigned int)r.cache_line_size, (unsigned int)r.latency_timer,
           (unsigned int)r.header_type, (unsigned int)r.bist,
           (unsigned int)r.irq_line, (unsigned int)r.irq_pin);
    for (index = 0u; index < r.bar_count; ++index)
        printf("    PCI %02Xh BAR%u raw=%08lXh (size not probed)\n",
               0x10u + index * 4u, index, (unsigned long)r.bars[index]);
    if (r.layout == 0u)
        printf("    PCI 2Ch SubsystemVendor=%04Xh SubsystemDevice=%04Xh\n",
               (unsigned int)r.subsystem_vendor,
               (unsigned int)r.subsystem_device);
    else if (r.layout == 1u)
        printf("    PCI 18h Primary=%u Secondary=%u Subordinate=%u "
               "SecondaryLatency=%u; 3Eh BridgeControl=%04Xh\n",
               (unsigned int)r.primary, (unsigned int)r.secondary,
               (unsigned int)r.subordinate,
               (unsigned int)r.secondary_latency,
               (unsigned int)r.bridge_control);
    switch (r.caps_status) {
    case WHP_PCI_CAP_HEADER_ONLY:
        printf("    PCI 34h CapPointer=%02Xh (header only; capabilities unread)\n",
               (unsigned int)r.caps_pointer);
        break;
    case WHP_PCI_CAP_COMPLETE:
        printf("    PCI 34h CapPointer=%02Xh (complete standard list)\n",
               (unsigned int)r.caps_pointer);
        if (r.pcie_cap_offset != 0u) {
            printf("    PCIe capability at %02Xh flags=%04Xh\n",
                   (unsigned int)r.pcie_cap_offset,
                   (unsigned int)r.pcie_flags);
            if (r.pcie_link_status_valid)
                printf("    PCIe %02Xh LinkStatus=%04Xh "
                       "speed-code=%u width=x%u DLL-active=%u "
                       "(snapshot only)\n",
                       (unsigned int)(r.pcie_cap_offset + 0x12u),
                       (unsigned int)r.pcie_link_status,
                       (unsigned int)(r.pcie_link_status & 0x0fu),
                       (unsigned int)((r.pcie_link_status >> 4) & 0x3fu),
                       (unsigned int)((r.pcie_link_status >> 13) & 1u));
        }
        break;
    case WHP_PCI_CAP_MALFORMED:
        puts("    PCI 34h capability chain malformed; no PCIe inference.");
        break;
    default:
        puts("    PCI status does not advertise standard capabilities.");
        break;
    }
}

/* Firmware memory/resource survey: no RAM patterns or memory allocation.
 * Reads BIOS INT 12h / INT 15h AH=88h and BDA metadata only.
 * The BAR comparison reuses the existing read-only PCI BIOS transport.
 */
static int report_memory_resources(int for_menu)
{
    struct whp_memory_firmware firmware;
    struct whp_memory_survey summary;
    union REGS inregs, outregs;
    const volatile unsigned short __far *bda_word;
    struct whp_pci95_bios_reader reader;
    enum whp_pci95_bios_result result;
    unsigned int last_bus, version, bus, dev, fn, multifunction, devfn;
    unsigned int count = 0u, observations = 0u, errors = 0u, printed = 0u;
    unsigned int limit = for_menu ? 3u : 24u;
    unsigned int index, max_bars;
    enum whp_memory_bar_notice notice;
    const char *interpretation;

    memset(&firmware, 0, sizeof(firmware));
    memset(&inregs, 0, sizeof(inregs));
    memset(&outregs, 0, sizeof(outregs));
    int86(0x12, &inregs, &outregs);
    firmware.int12_kb = (uint16_t)outregs.w.ax;

    /* 0040:0013 is an ODD byte offset: never index a word array with 13/2. */
    bda_word = (const volatile unsigned short __far *)MK_FP(0x0040u, 0x0013u);
    firmware.bda_kb = (uint16_t)*bda_word;
    bda_word = (const volatile unsigned short __far *)MK_FP(0x0040u, 0x000eu);
    firmware.ebda_segment = (uint16_t)*bda_word;

    memset(&inregs, 0, sizeof(inregs));
    memset(&outregs, 0, sizeof(outregs));
    inregs.h.ah = 0x88u;
    int86(0x15, &inregs, &outregs);
    if (!outregs.w.cflag) {
        /* AH is part of the returned 16-bit KiB value, NOT a status byte. */
        firmware.int15_88_valid = 1u;
        firmware.int15_88_kb = (uint16_t)outregs.w.ax;
    }

    if (!whp_memory_survey_decode(&firmware, &summary))
        return 2;
    puts("Memory resource survey (BIOS metadata; no RAM test)");
    if (summary.conventional_valid)
        printf("  INT 12h conventional boundary: %u KiB (%05lXh)\n",
               (unsigned int)firmware.int12_kb,
               (unsigned long)summary.conventional_end);
    else
        printf("  INT 12h boundary: %u KiB (untrusted PC range)\n",
               (unsigned int)firmware.int12_kb);
    if (summary.bda_valid)
        printf("  BDA 0040:0013h: %u KiB%s\n",
               (unsigned int)firmware.bda_kb,
               summary.bda_mismatch ? " [DISAGREES WITH INT 12h]" : "");
    else
        puts("  BDA conventional-memory size unavailable/implausible.");
    if (summary.ebda_candidate)
        printf("  EBDA location candidate: %05lXh%s\n",
               (unsigned long)summary.ebda_base,
               summary.ebda_below_conventional_end ?
               " [DISAGREES WITH CONVENTIONAL BOUND]" : "");
    else
        puts("  EBDA unknown; 0040:000Eh can instead mean LPT4.");
    if (summary.legacy_extended_valid)
        printf("  INT 15h/88h legacy extended-memory estimate: %u KiB\n",
               (unsigned int)summary.legacy_extended_kb);
    else
        puts("  INT 15h/88h extended-memory estimate unavailable.");
    puts("  A0000h-BFFFFh: legacy video; C0000h-FFFFFh: ROM/UMB.");
    puts("  These are NOT verified physical-RAM allocations.");

    if (!summary.conventional_valid) {
        puts("  PCI overlap scan unavailable: conventional bound untrusted.");
        return 0;
    }
    puts("  PCI memory BAR base check (sizes unknown, read only):");
    if (!pci_bios_installed(&last_bus, &version)) {
        puts("  No usable PCI BIOS; memory/PCI comparison unavailable.");
        return 0;
    }
    reader.read_word = bios_word;
    reader.context = 0;
    for (bus = 0u; bus <= last_bus; ++bus) {
        for (dev = 0u; dev < 32u; ++dev) {
            multifunction = 0u;
            for (fn = 0u; fn < 8u; ++fn) {
                if (fn != 0u && !multifunction)
                    break;
                devfn = (dev << 3) | fn;
                result = whp_pci95_bios_read_config(&reader, (uint8_t)bus,
                           (uint8_t)devfn, config_bytes, 64u);
                if (result == WHP_PCI95_BIOS_NO_DEVICE) {
                    if (fn == 0u)
                        break;
                    continue;
                }
                if (result != WHP_PCI95_BIOS_OK) {
                    ++errors;
                    if (fn == 0u)
                        break;
                    continue;
                }
                if (fn == 0u)
                    multifunction = (config_bytes[0x0eu] & 0x80u) != 0u;
                ++count;
                /* Disabled memory decode and I/O BARs are not active
                 * MMIO mappings. The BAR parser never performs sizing writes.
                 */
                if (!(config_bytes[4] & 0x02u))
                    continue;
                max_bars = (config_bytes[0x0eu] & 0x7fu) == 0u ? 6u :
                           (config_bytes[0x0eu] & 0x7fu) == 1u ? 2u : 0u;
                for (index = 0u; index < max_bars; ++index) {
                    int bar_status = whp_pci_dos_read_bar(
                        config_bytes, 64u, index, &pci_bar);
                    if (bar_status != 1)
                        continue;
                    notice = whp_memory_bar_check(&summary,
                               pci_bar.base, pci_bar.is_io, 1);
                    if (pci_bar.slots == 2u)
                        ++index;
                    if (notice == WHP_MEM_BAR_NONE)
                        continue;
                    ++observations;
                    if (printed >= limit)
                        continue;
                    switch (notice) {
                    case WHP_MEM_BAR_CONVENTIONAL:
                        interpretation = "POTENTIAL conventional RAM conflict";
                        break;
                    case WHP_MEM_BAR_EBDA_CANDIDATE:
                        interpretation = "possible EBDA-area base";
                        break;
                    case WHP_MEM_BAR_LOW_GAP:
                        interpretation = "low memory gap, allocation unknown";
                        break;
                    default:
                        interpretation = "legacy VGA/ROM/UMB (may be normal)";
                        break;
                    }
                    printf("  PCI %u:%02u.%u BAR%u=%08lX%08lXh: %s\n",
                           bus, dev, fn, (unsigned int)pci_bar.index,
                           (unsigned long)(uint32_t)(pci_bar.base >> 32),
                           (unsigned long)(uint32_t)pci_bar.base,
                           interpretation);
                    ++printed;
                }
            }
        }
    }
    printf("  PCI functions=%u, observations=%u, read errors=%u\n",
           count, observations, errors);
    if (observations > printed)
        printf("  %u observations omitted (use /M for more).\n",
               observations - printed);
    puts("  No E820 map, BAR sizes, DMA buffers, RAM health or");
    puts("  installed-driver ownership verified; hints are not proof.");
    return errors ? 1 : 0;
}

static int run_report(int show_all_pci)
{
    struct whp_pci95_bios_reader reader;
    enum whp_pci95_bios_result result;
    unsigned int last_bus, version, bus, dev, fn, multifn;
    unsigned int found_bridges = 0, found_nics = 0, found_hda = 0, found_storage = 0, found_usb = 0, found_isa_bridges = 0, read_errors = 0;
    unsigned int snapshot_size, devfn;
    unsigned int id_vendor, id_device, cls, subclass;
    int wanted_bridge, wanted_nic, wanted_hda, wanted_storage, wanted_usb, wanted_isa_bridge;
    unsigned int conventional_kb;

    report_dos_version();
    probe_legacy_bios();
    conventional_kb = bios_conventional_memory_kb();
    if (conventional_kb)
        printf("WHPPROBE: BIOS conventional-memory boundary=%u KiB (not free RAM)\n",
               conventional_kb);
    else
        puts("WHPPROBE: BIOS conventional-memory boundary unavailable.");
    if (!pci_bios_installed(&last_bus, &version)) {
        puts("WHPPROBE: no usable INT 1Ah PCI BIOS (B101h).");
        puts("Legacy BIOS inventory above is available; no PCI reads attempted.");
        puts("Missing PCI BIOS does not prove PCIe or ISA hardware is absent.");
        return 2;
    }
    printf("WHPPROBE: PCI BIOS version %02Xh, last reported bus %u\n",
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
                wanted_isa_bridge = whp_dos_legacy_pci_isa_bridge(
                    config_bytes, 64u);
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
                    !wanted_storage && !wanted_usb && !wanted_isa_bridge) {
                    if (show_all_pci) {
                        printf("%u:%02u.%u PCI %04X:%04X class=%02X:%02X:%02X\n",
                               bus, dev, fn, id_vendor, id_device,
                               cls, subclass, (unsigned int)config_bytes[9]);
                        print_pci_registers(config_bytes, 64u);
                    } else if (cls == 0x06u && subclass == 0u) {
                        printf("%u:%02u.%u host bridge %04X:%04X (not PCI type-1)\n",
                               bus, dev, fn, id_vendor, id_device);
                    }
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
                if (wanted_isa_bridge) {
                    ++found_isa_bridges;
                    printf("%u:%02u.%u ISA/LPC bridge class 06:01 %04X:%04X\n",
                           bus, dev, fn, id_vendor, id_device);
                    puts("    PCI class only; legacy bus and physical ISA slots unverified.");
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
                print_pci_registers(config_bytes, snapshot_size);
            }
        }
    }
    printf("WHPPROBE: PCI %u bridge(s), %u ISA/LPC bridge candidate(s), %u RTL8168/8111 candidate(s), %u HDA candidate(s), %u storage controller(s), %u USB HCI(s), %u read error(s).\n",
           found_bridges, found_isa_bridges, found_nics, found_hda, found_storage, found_usb, read_errors);
    puts("A DOS PCI BIOS result is not Windows 95 PCI.VXD/NDIS validation.");
    return read_errors ? 1 : 0;
}

/* Only interactive screen presentation touches video BIOS. Redirection,
 * report printing and the original PCI read-only transport stay unchanged.
 */
static void menu_clear(void)
{
    union REGS inregs, outregs;
    unsigned int mode, columns;
    fflush(stdout);
    memset(&inregs, 0, sizeof(inregs));
    memset(&outregs, 0, sizeof(outregs));
    inregs.h.ah = 0x0fu;
    int86(0x10, &inregs, &outregs);
    mode = outregs.h.al & 0x7fu;
    columns = outregs.h.ah;
    if (mode > 3u && mode != 7u) {
        puts("\n----------------------------------------");
        return;
    }
    memset(&inregs, 0, sizeof(inregs));
    inregs.w.ax = 0x0600u;
    inregs.h.bh = 7u;
    inregs.w.cx = 0u;
    inregs.h.dh = 24u;
    inregs.h.dl = (uint8_t)(columns > 0u && columns <= 80u ?
                            columns - 1u : 79u);
    int86(0x10, &inregs, &outregs);
    memset(&inregs, 0, sizeof(inregs));
    inregs.h.ah = 2u;
    inregs.w.dx = 0u;
    int86(0x10, &inregs, &outregs);
}

static int menu_read_key(void)
{
    int key;
    fflush(stdout);
    key = getch();
    if (key == 0 || key == 0xe0)
        return 0x100 | getch();
    return key;
}

static void menu_header(const char *title)
{
    puts("WHP Diagnostics - DOS System Probe");
    puts("==================================");
    puts(title);
    puts("----------------------------------");
}

static void menu_pause(void)
{
    puts("\n[Enter/Esc] Return to menu");
    for (;;) {
        int key = menu_read_key();
        if (key == 13 || key == 27)
            return;
    }
}

/* Display only one matching PCI function, and defer its 256-byte
 * configuration read until requested. Never dereference BAR addresses.
 */
static int menu_device(const struct whp_pci95_bios_reader *reader,
                       unsigned int bus, unsigned int dev,
                       unsigned int fn, unsigned int index)
{
    unsigned int vendor, device, snapshot_size;
    enum whp_pci95_bios_result result;
    int key;
    vendor = (unsigned int)config_bytes[0] |
             ((unsigned int)config_bytes[1] << 8);
    device = (unsigned int)config_bytes[2] |
             ((unsigned int)config_bytes[3] << 8);
    menu_clear();
    menu_header("PCI function summary");
    printf("Device %u: bus %u, device %u, function %u\n",
           index, bus, dev, fn);
    printf("PCI ID %04X:%04X    Revision %02Xh\n",
           vendor, device, (unsigned int)config_bytes[8]);
    printf("Class %02X:%02X:%02X    Header %02Xh\n",
           (unsigned int)config_bytes[11], (unsigned int)config_bytes[10],
           (unsigned int)config_bytes[9], (unsigned int)config_bytes[14]);
    puts("A PCI identity does not establish a working driver.");
    puts("\n[R] Registers  [Enter/N] Next device  [Esc/Q] Menu");
    for (;;) {
        key = menu_read_key();
        if (key == 27 || key == 'q' || key == 'Q')
            return 0;
        if (key == 13 || key == 'n' || key == 'N')
            return 1;
        if (key == 'r' || key == 'R')
            break;
    }

    /* The transport preserves the original 64-byte header on failure. */
    snapshot_size = 64u;
    result = whp_pci95_bios_read_config(reader, (uint8_t)bus,
                 (uint8_t)((dev << 3) | fn), config_bytes, 256u);
    if (result == WHP_PCI95_BIOS_OK)
        snapshot_size = 256u;
    menu_clear();
    menu_header("Standard PCI configuration registers");
    printf("%u:%02u.%u  ID %04X:%04X\n", bus, dev, fn, vendor, device);
    if (snapshot_size == 64u)
        puts("Extended snapshot unavailable: header registers only.");
    print_pci_registers(config_bytes, snapshot_size);
    puts("\n[Enter/N] Next device  [Esc/Q] Menu");
    for (;;) {
        key = menu_read_key();
        if (key == 27 || key == 'q' || key == 'Q')
            return 0;
        if (key == 13 || key == 'n' || key == 'N')
            return 1;
    }
}

static void menu_pci(unsigned int section)
{
    struct whp_pci95_bios_reader reader;
    enum whp_pci95_bios_result result;
    unsigned int last_bus, version, bus, dev, fn, multifn, devfn;
    unsigned int matches = 0u, errors = 0u;
    menu_clear();
    menu_header(whp_probe_menu_label(section));
    puts("Reading PCI BIOS configuration headers...");
    fflush(stdout);
    if (!pci_bios_installed(&last_bus, &version)) {
        puts("No usable PCI BIOS service. Hardware presence is unknown.");
        menu_pause();
        return;
    }
    reader.read_word = bios_word;
    reader.context = 0;
    for (bus = 0u; bus <= last_bus; ++bus) {
        for (dev = 0u; dev < 32u; ++dev) {
            multifn = 0u;
            for (fn = 0u; fn < 8u; ++fn) {
                if (fn && !multifn)
                    break;
                devfn = (dev << 3) | fn;
                result = whp_pci95_bios_read_config(&reader, (uint8_t)bus,
                                (uint8_t)devfn, config_bytes, 64u);
                if (result == WHP_PCI95_BIOS_NO_DEVICE) {
                    if (!fn)
                        break;
                    continue;
                }
                if (result != WHP_PCI95_BIOS_OK) {
                    ++errors;
                    if (!fn)
                        break;
                    continue;
                }
                if (!fn)
                    multifn = (config_bytes[14] & 0x80u) != 0u;
                if (!whp_probe_menu_matches(section, config_bytes, 64u))
                    continue;
                ++matches;
                if (!menu_device(&reader, bus, dev, fn, matches))
                    return;
            }
        }
    }
    menu_clear();
    menu_header(whp_probe_menu_label(section));
    printf("Completed: %u match(es), %u read error(s).\n", matches, errors);
    puts("Neither PCI configuration nor device state was modified.");
    menu_pause();
}

static int menu_main(void)
{
    unsigned int selected = 1u, i;
    enum whp_probe_menu_action action;
    for (;;) {
        menu_clear();
        menu_header("Main menu");
        for (i = 1u; i <= 9u; ++i)
            printf(" %c %u. %s\n", i == selected ? '>' : ' ',
                   i, whp_probe_menu_label(i));
        puts("\n[Up/Down] Select [Enter/1-9] Open [Esc/0] Exit");
        puts("Batch reports: WHPPROBE /R, /A or /M");
        puts("Read-only inspection; no driver installation.");
        action = whp_probe_menu_key(&selected, menu_read_key());
        if (action == WHP_MENU_EXIT) {
            menu_clear();
            return 0;
        }
        if (action != WHP_MENU_OPEN)
            continue;
        if (selected == 1u) {
            unsigned int kb;
            menu_clear();
            menu_header(whp_probe_menu_label(selected));
            report_dos_version();
            kb = bios_conventional_memory_kb();
            if (kb)
                printf("Conventional memory boundary: %u KiB\n", kb);
            else
                puts("Conventional memory boundary unavailable.");
            puts("Boundary does not measure free RAM.");
            puts("\n[M] Memory/PCI resource survey   [Enter/Esc] Menu");
            for (;;) {
                int key = menu_read_key();
                if (key == 13 || key == 27)
                    break;
                if (key == 'm' || key == 'M') {
                    menu_clear();
                    menu_header("Memory and PCI resource observations");
                    report_memory_resources(1);
                    menu_pause();
                    break;
                }
            }
        } else if (selected == 2u) {
            menu_clear();
            menu_header(whp_probe_menu_label(selected));
            probe_legacy_bios();
            menu_pause();
        } else {
            menu_pci(selected);
        }
    }
}

int main(int argc, char **argv)
{
    if (argc == 2 && (strcmp(argv[1], "/M") == 0 ||
                      strcmp(argv[1], "--memory") == 0))
        return report_memory_resources(0);
    if (argc == 2 && (strcmp(argv[1], "/R") == 0 ||
                      strcmp(argv[1], "--report") == 0))
        return run_report(0);
    if (argc == 2 && (strcmp(argv[1], "/A") == 0 ||
                      strcmp(argv[1], "--all-pci") == 0))
        return run_report(1);
    if (argc == 2 && (strcmp(argv[1], "/?") == 0 ||
                      strcmp(argv[1], "--help") == 0)) {
        puts("WHPPROBE [/R|/A|/M|/?]");
        puts("No option: DOS menu when interactive; report if redirected.");
        puts("/R = standard report; /A = all PCI; /M = memory survey.");
        return 0;
    }
    if (argc != 1) {
        puts("Invalid option. Run WHPPROBE /?");
        return 2;
    }
    /* Preserve WHPPROBE > PCI.TXT without screen clearing or key input. */
    if (!isatty(fileno(stdin)) || !isatty(fileno(stdout)))
        return run_report(0);
    return menu_main();
}
