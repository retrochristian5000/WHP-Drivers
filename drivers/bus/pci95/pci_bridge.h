/* Read-only PCI type-1 bridge analysis for WHP-Drivers.
 * Pure decoding of caller-supplied bytes: no PCI access, PnP, IRQ or writes.
 * Not a Win95 PCI.VXD replacement or a runnable bridge driver.
 */
#ifndef WHP_PCI95_BRIDGE_H
#define WHP_PCI95_BRIDGE_H
#include <stddef.h>
#include <stdint.h>

#define WHP_PCI95_BRIDGE_HEADER_BYTES 64u

enum whp_pci95_parse_result {
    WHP_PCI95_PARSE_OK = 0,
    WHP_PCI95_PARSE_BAD_INPUT,
    WHP_PCI95_PARSE_NO_DEVICE,
    WHP_PCI95_PARSE_NOT_BRIDGE
};
enum whp_pci95_cap_status {
    WHP_PCI95_CAPS_NOT_ADVERTISED = 0,
    WHP_PCI95_CAPS_COMPLETE,
    WHP_PCI95_CAPS_TRUNCATED,
    WHP_PCI95_CAPS_MALFORMED
};
enum whp_pci95_route_status {
    WHP_PCI95_ROUTE_IN_DECLARED_RANGE = 0,
    WHP_PCI95_ROUTE_BAD_INPUT,
    WHP_PCI95_ROUTE_UNCONFIGURED,
    WHP_PCI95_ROUTE_PRIMARY_MISMATCH,
    WHP_PCI95_ROUTE_OUTSIDE_RANGE
};
struct whp_pci95_bridge {
    uint16_t vendor, device, command, status;
    uint16_t bridge_control;
    uint8_t revision, class_code, subclass, prog_if;
    uint8_t multifunction;
    uint8_t primary, secondary, subordinate;
    uint8_t irq_line, irq_pin;
    uint32_t bar0_raw, bar1_raw;
    uint32_t io_base, io_limit;
    uint32_t mem_base, mem_limit;
    uint64_t pref_base, pref_limit;
    uint8_t io_window_present, mem_window_present, pref_window_present;
    enum whp_pci95_cap_status caps;
    uint8_t pcie_cap_offset; /* Populated only for a complete valid chain. */
};

/* Requires a raw 64-byte or longer PCI configuration-space snapshot.
 * Output is unchanged unless the type-1 PCI-to-PCI bridge parses correctly.
 * An observed window does NOT prove routing, memory decode or ownership.
 */
enum whp_pci95_parse_result whp_pci95_decode_bridge(
    const uint8_t *cfg, size_t length, struct whp_pci95_bridge *out);

/* This is a consistency check on reported bus numbers, not an actual scan.
 * It cannot prove that an endpoint is electrically connected or accessible.
 */
enum whp_pci95_route_status whp_pci95_check_bus_range(
    const struct whp_pci95_bridge *bridge,
    unsigned int observed_parent_bus, unsigned int endpoint_bus);
#endif
