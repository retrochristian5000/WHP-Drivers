#include "pci_config_report.h"
#include <string.h>

static uint16_t rd16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void decode_capabilities(const uint8_t *cfg, size_t length,
                                struct whp_pci_config_report *r)
{
    uint8_t visited[64];
    unsigned int at, steps, pcie = 0u;
    memset(visited, 0, sizeof(visited));
    if (!(r->status & 0x0010u) || r->layout > 1u) {
        r->caps_status = WHP_PCI_CAP_NOT_ADVERTISED;
        return;
    }
    r->caps_pointer = cfg[0x34u];
    if (r->caps_pointer == 0u) {
        r->caps_status = WHP_PCI_CAP_MALFORMED;
        return;
    }
    if (length == 64u) {
        r->caps_status = WHP_PCI_CAP_HEADER_ONLY;
        return;
    }
    at = r->caps_pointer;
    for (steps = 0u; steps < 48u; ++steps) {
        unsigned int next;
        if (at < 0x40u || at > 0xfcu || (at & 3u) ||
            visited[at >> 2] || at + 2u > length ||
            cfg[at] == 0u || cfg[at] == 0xffu) {
            r->caps_status = WHP_PCI_CAP_MALFORMED;
            return;
        }
        visited[at >> 2] = 1u;
        if (cfg[at] == 0x10u)
            pcie = at;
        next = cfg[at + 1u];
        if (next == 0u) {
            r->caps_status = WHP_PCI_CAP_COMPLETE;
            break;
        }
        at = next;
    }
    if (r->caps_status != WHP_PCI_CAP_COMPLETE) {
        r->caps_status = WHP_PCI_CAP_MALFORMED;
        return;
    }
    if (pcie == 0u || pcie + 4u > length)
        return;
    r->pcie_cap_offset = (uint8_t)pcie;
    r->pcie_flags = rd16(cfg + pcie + 2u);
    /* RC integrated endpoints and event collectors do not have a link
     * register group. Only interpret Link Status for link-bearing types.
     */
    {
        unsigned int type = (unsigned int)((r->pcie_flags >> 4) & 0x0fu);
        if ((type == 0u || type == 1u ||
             (type >= 4u && type <= 8u)) &&
            pcie + 0x14u <= length) {
            r->pcie_link_status = rd16(cfg + pcie + 0x12u);
            r->pcie_link_status_valid = 1u;
        }
    }
}

int whp_pci_config_report_decode(const uint8_t *cfg, size_t length,
                                 struct whp_pci_config_report *out)
{
    struct whp_pci_config_report r;
    unsigned int i;
    if (cfg == 0 || out == 0 || (length != 64u && length != 256u))
        return 0;
    if (rd16(cfg) == 0u || rd16(cfg) == 0xffffu)
        return 0;
    memset(&r, 0, sizeof(r));
    r.vendor = rd16(cfg);
    r.device = rd16(cfg + 0x02u);
    r.command = rd16(cfg + 0x04u);
    r.status = rd16(cfg + 0x06u);
    r.revision = cfg[0x08u];
    r.prog_if = cfg[0x09u];
    r.subclass = cfg[0x0au];
    r.class_code = cfg[0x0bu];
    r.cache_line_size = cfg[0x0cu];
    r.latency_timer = cfg[0x0du];
    r.header_type = cfg[0x0eu];
    r.layout = (uint8_t)(r.header_type & 0x7fu);
    r.bist = cfg[0x0fu];
    r.irq_line = cfg[0x3cu];
    r.irq_pin = cfg[0x3du];
    if (r.layout == 0u) {
        r.bar_count = 6u;
        r.subsystem_vendor = rd16(cfg + 0x2cu);
        r.subsystem_device = rd16(cfg + 0x2eu);
    } else if (r.layout == 1u) {
        r.bar_count = 2u;
        r.primary = cfg[0x18u];
        r.secondary = cfg[0x19u];
        r.subordinate = cfg[0x1au];
        r.secondary_latency = cfg[0x1bu];
        r.bridge_control = rd16(cfg + 0x3eu);
    }
    for (i = 0u; i < (unsigned int)r.bar_count; ++i)
        r.bars[i] = rd32(cfg + 0x10u + (size_t)i * 4u);
    decode_capabilities(cfg, length, &r);
    *out = r;
    return 1;
}
