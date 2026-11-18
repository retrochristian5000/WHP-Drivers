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
#include "rtl8111e.h"

static uint8_t config_bytes[WHP_PCI95_BIOS_CONFIG_SIZE];
static struct whp_pci95_bridge bridge_info;
static struct whp_rtl8111e_pci_snapshot nic_info;

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

int main(void)
{
    struct whp_pci95_bios_reader reader;
    enum whp_pci95_bios_result result;
    unsigned int last_bus, version, bus, dev, fn, multifn;
    unsigned int found_bridges = 0, found_nics = 0, found_hda = 0, read_errors = 0;
    unsigned int snapshot_size, devfn;
    unsigned int id_vendor, id_device, cls, subclass;
    int wanted_bridge, wanted_nic, wanted_hda;

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
                if (!wanted_bridge && !wanted_nic && !wanted_hda) {
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
                if (wanted_bridge &&
                    whp_pci95_decode_bridge(config_bytes, snapshot_size,
                                            &bridge_info) == WHP_PCI95_PARSE_OK) {
                    ++found_bridges;
                    printf("%u:%02u.%u BRIDGE %04X:%04X primary=%u child=%u last=%u\n",
                           bus, dev, fn, id_vendor, id_device,
                           (unsigned int)bridge_info.primary,
                           (unsigned int)bridge_info.secondary,
                           (unsigned int)bridge_info.subordinate);
                    printf("    PCI command %04Xh; memory window %s; bus 3 %s\n",
                           (unsigned int)bridge_info.command,
                           bridge_info.mem_window_present ? "declared" : "disabled/invalid",
                           whp_pci95_check_bus_range(&bridge_info, bus, 3u) ==
                               WHP_PCI95_ROUTE_IN_DECLARED_RANGE ?
                               "in reported range" : "not in reported range");
                    print_caps("bridge", (int)bridge_info.caps,
                               (unsigned int)bridge_info.pcie_cap_offset);
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
            }
        }
    }
    printf("WHPPCI: %u bridge(s), %u RTL8168/8111 candidate(s), %u HDA candidate(s), %u read error(s).\n",
           found_bridges, found_nics, found_hda, read_errors);
    puts("A DOS PCI BIOS result is not Windows 95 PCI.VXD/NDIS validation.");
    return read_errors ? 1 : 0;
}
