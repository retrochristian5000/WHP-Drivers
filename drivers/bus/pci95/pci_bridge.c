#include "pci_bridge.h"

static uint16_t rd16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}
static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* Never follow standard PCI capability pointers outside 0x40..0xff. */
static enum whp_pci95_cap_status scan_capabilities(
    const uint8_t *cfg, size_t length, uint8_t *pcie)
{
    uint8_t visited[64] = {0};
    uint8_t at = cfg[0x34];
    uint8_t candidate = 0;
    unsigned int steps;
    if (at == 0)
        return WHP_PCI95_CAPS_MALFORMED;
    for (steps = 0; steps < 48u; ++steps) {
        uint8_t next, id;
        if (at < 0x40u || at > 0xfcu || (at & 3u))
            return WHP_PCI95_CAPS_MALFORMED;
        if (visited[at >> 2])
            return WHP_PCI95_CAPS_MALFORMED;
        if ((size_t)at + 2u > length)
            return WHP_PCI95_CAPS_TRUNCATED;
        visited[at >> 2] = 1;
        id = cfg[at];
        next = cfg[at + 1u];
        if (id == 0u || id == 0xffu)
            return WHP_PCI95_CAPS_MALFORMED;
        if (id == 0x10u)
            candidate = at; /* Standard PCI Express capability ID. */
        if (next == 0) {
            *pcie = candidate;
            return WHP_PCI95_CAPS_COMPLETE;
        }
        at = next;
    }
    return WHP_PCI95_CAPS_MALFORMED;
}

enum whp_pci95_parse_result whp_pci95_decode_bridge(
    const uint8_t *cfg, size_t length, struct whp_pci95_bridge *out)
{
    struct whp_pci95_bridge b = {0};
    uint8_t io_base_type, io_limit_type;
    uint16_t pbase, plimit;
    unsigned int io_type, pref_type;
    if (!cfg || !out || length < WHP_PCI95_BRIDGE_HEADER_BYTES)
        return WHP_PCI95_PARSE_BAD_INPUT;
    b.vendor = rd16(cfg);
    b.device = rd16(cfg + 2u);
    if (b.vendor == 0xffffu || b.vendor == 0)
        return WHP_PCI95_PARSE_NO_DEVICE;
    b.revision = cfg[8];
    b.prog_if = cfg[9];
    b.subclass = cfg[10];
    b.class_code = cfg[11];
    b.multifunction = (uint8_t)((cfg[14] & 0x80u) != 0u);
    if ((cfg[14] & 0x7fu) != 1u || b.class_code != 0x06u ||
        b.subclass != 0x04u)
        return WHP_PCI95_PARSE_NOT_BRIDGE;

    b.command = rd16(cfg + 4u);
    b.status = rd16(cfg + 6u);
    b.bar0_raw = rd32(cfg + 0x10);
    b.bar1_raw = rd32(cfg + 0x14);
    b.primary = cfg[0x18];
    b.secondary = cfg[0x19];
    b.subordinate = cfg[0x1a];
    b.irq_line = cfg[0x3c];
    b.irq_pin = cfg[0x3d];
    b.bridge_control = rd16(cfg + 0x3e);

    /* PCI type-1 memory window has 1 MiB granularity. */
    b.mem_base = ((uint32_t)(rd16(cfg + 0x20) & 0xfff0u)) << 16;
    b.mem_limit = (((uint32_t)(rd16(cfg + 0x22) & 0xfff0u)) << 16) |
                  0x000fffffu;
    b.mem_window_present = (uint8_t)(b.mem_base <= b.mem_limit);

    /* Prefetchable window may use 64-bit extension registers. */
    pbase = rd16(cfg + 0x24);
    plimit = rd16(cfg + 0x26);
    pref_type = (unsigned int)(pbase & 0x0fu);
    if (pref_type == (unsigned int)(plimit & 0x0fu) && pref_type <= 1u) {
        b.pref_base = ((uint64_t)(pbase & 0xfff0u)) << 16;
        b.pref_limit = (((uint64_t)(plimit & 0xfff0u)) << 16) |
                       UINT64_C(0x000fffff);
        if (pref_type == 1u) {
            b.pref_base |= ((uint64_t)rd32(cfg + 0x28)) << 32;
            b.pref_limit |= ((uint64_t)rd32(cfg + 0x2c)) << 32;
        }
        b.pref_window_present = (uint8_t)(b.pref_base <= b.pref_limit);
    }

    /* I/O bridge windows have 4 KiB granularity, optionally 32-bit. */
    io_base_type = (uint8_t)(cfg[0x1c] & 0x0fu);
    io_limit_type = (uint8_t)(cfg[0x1d] & 0x0fu);
    io_type = io_base_type;
    if (io_type == io_limit_type && io_type <= 1u) {
        b.io_base = ((uint32_t)(cfg[0x1c] & 0xf0u)) << 8;
        b.io_limit = (((uint32_t)(cfg[0x1d] & 0xf0u)) << 8) | 0xfffu;
        if (io_type == 1u) {
            b.io_base |= ((uint32_t)rd16(cfg + 0x30)) << 16;
            b.io_limit |= ((uint32_t)rd16(cfg + 0x32)) << 16;
        }
        b.io_window_present = (uint8_t)(b.io_base <= b.io_limit);
    }

    if (b.status & 0x0010u)
        b.caps = scan_capabilities(cfg, length, &b.pcie_cap_offset);
    else
        b.caps = WHP_PCI95_CAPS_NOT_ADVERTISED;
    *out = b;
    return WHP_PCI95_PARSE_OK;
}

enum whp_pci95_route_status whp_pci95_check_bus_range(
    const struct whp_pci95_bridge *bridge,
    unsigned int observed_parent_bus, unsigned int endpoint_bus)
{
    if (!bridge || observed_parent_bus > 255u || endpoint_bus > 255u)
        return WHP_PCI95_ROUTE_BAD_INPUT;
    if (!bridge->secondary || bridge->primary >= bridge->secondary ||
        bridge->subordinate < bridge->secondary)
        return WHP_PCI95_ROUTE_UNCONFIGURED;
    if (bridge->primary != observed_parent_bus)
        return WHP_PCI95_ROUTE_PRIMARY_MISMATCH;
    if (endpoint_bus < bridge->secondary || endpoint_bus > bridge->subordinate)
        return WHP_PCI95_ROUTE_OUTSIDE_RANGE;
    return WHP_PCI95_ROUTE_IN_DECLARED_RANGE;
}
