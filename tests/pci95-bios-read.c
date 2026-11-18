/* Native, fabricated PCI BIOS transport tests. Never touches hardware. */
#include "pci_bios_read.h"
#include "pci_bridge.h"
#include <stdio.h>
#include <string.h>

#define CHECK(e) do { ++checks; if (!(e)) { \
    fprintf(stderr, "PCI BIOS test failed line %d: %s\n", __LINE__, #e); \
    return 1; } } while (0)

struct mock {
    uint8_t cfg[256];
    unsigned int calls;
    int fail_at;
    int missing;
};
static void put16(uint8_t *p, unsigned int off, uint16_t v)
{
    p[off] = (uint8_t)v;
    p[off + 1u] = (uint8_t)(v >> 8);
}
static int read_word(void *context, uint8_t bus, uint8_t devfn,
                     uint8_t offset, uint16_t *value)
{
    struct mock *m = (struct mock *)context;
    ++m->calls;
    if (bus != 3u || devfn != 0u || m->missing) return 0;
    if ((int)offset == m->fail_at) return -1;
    *value = (uint16_t)(m->cfg[offset] | ((uint16_t)m->cfg[offset + 1u] << 8));
    return 1;
}
int main(void)
{
    struct mock m;
    struct whp_pci95_bios_reader reader;
    struct whp_pci95_bridge b;
    uint8_t output[256];
    unsigned int i;
    int checks = 0;
    memset(&m, 0, sizeof(m));
    memset(m.cfg, 0, sizeof(m.cfg));
    m.fail_at = -1;
    put16(m.cfg, 0, 0x1022u);
    put16(m.cfg, 2, 0x4311u);
    m.cfg[0x0au] = 0x04u; m.cfg[0x0bu] = 0x06u;
    m.cfg[0x0eu] = 0x01u;
    m.cfg[0x18u] = 0; m.cfg[0x19u] = 3; m.cfg[0x1au] = 3;
    put16(m.cfg, 0x20u, 0x8000u);
    put16(m.cfg, 0x22u, 0x8ff0u);
    reader.read_word = read_word; reader.context = &m;
    memset(output, 0xa5, sizeof(output));
    CHECK(whp_pci95_bios_read_config(0, 3, 0, output, 64) == WHP_PCI95_BIOS_BAD_INPUT);
    CHECK(whp_pci95_bios_read_config(&reader, 3, 0, 0, 64) == WHP_PCI95_BIOS_BAD_INPUT);
    CHECK(whp_pci95_bios_read_config(&reader, 3, 0, output, 65) == WHP_PCI95_BIOS_BAD_INPUT);
    CHECK(whp_pci95_bios_read_config(&reader, 3, 0, output, 257) == WHP_PCI95_BIOS_BAD_INPUT);
    CHECK(whp_pci95_bios_read_config(&reader, 3, 0, output, 64) == WHP_PCI95_BIOS_OK);
    CHECK(m.calls == 32u && output[0] == 0x22u && output[1] == 0x10u);
    CHECK(output[0x19] == 3u && output[0x1a] == 3u);
    CHECK(whp_pci95_decode_bridge(output, 64, &b) == WHP_PCI95_PARSE_OK);
    CHECK(whp_pci95_check_bus_range(&b, 0, 3) == WHP_PCI95_ROUTE_IN_DECLARED_RANGE);
    m.calls = 0;
    CHECK(whp_pci95_bios_read_config(&reader, 3, 0, output, 256) == WHP_PCI95_BIOS_OK);
    CHECK(m.calls == 128u);
    m.fail_at = 0x20;
    memset(output, 0x5a, sizeof(output));
    CHECK(whp_pci95_bios_read_config(&reader, 3, 0, output, 64) == WHP_PCI95_BIOS_READ_FAILED);
    for (i = 0; i < sizeof(output); ++i) CHECK(output[i] == 0x5a);
    m.fail_at = 0;
    CHECK(whp_pci95_bios_read_config(&reader, 3, 0, output, 64) == WHP_PCI95_BIOS_READ_FAILED);
    m.fail_at = -1; m.missing = 1;
    CHECK(whp_pci95_bios_read_config(&reader, 3, 0, output, 64) == WHP_PCI95_BIOS_NO_DEVICE);
    m.missing = 0;
    CHECK(whp_pci95_bios_read_config(&reader, 2, 0, output, 64) == WHP_PCI95_BIOS_NO_DEVICE);
    put16(m.cfg, 0, 0xffffu);
    CHECK(whp_pci95_bios_read_config(&reader, 3, 0, output, 64) == WHP_PCI95_BIOS_NO_DEVICE);
    printf("PCI95 BIOS transport: %d synthetic checks passed\n", checks);
    return 0;
}
