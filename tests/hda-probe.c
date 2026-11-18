/* HDA identification tests: synthetic PCI and controller MMIO images. */
#include "hda_probe.h"
#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { ++checks; if (!(expr)) { \
    fprintf(stderr, "HDA probe failed line %d: %s\n", __LINE__, #expr); \
    return 1; } } while (0)
static void put16(uint8_t *p, unsigned int at, uint16_t v)
{
    p[at] = (uint8_t)v;
    p[at + 1u] = (uint8_t)(v >> 8);
}
static void put32(uint8_t *p, unsigned int at, uint32_t v)
{
    put16(p, at, (uint16_t)v);
    put16(p, at + 2u, (uint16_t)(v >> 16));
}
static void pci_fixture(uint8_t cfg[64])
{
    memset(cfg, 0, 64u);
    put16(cfg, 0, 0x1022u);
    put16(cfg, 2, 0x780du);
    put16(cfg, 4, 0x0006u);
    cfg[8] = 1u;
    cfg[9] = WHP_HDA_PCI_PROGIF;
    cfg[10] = WHP_HDA_PCI_SUBCLASS;
    cfg[11] = WHP_HDA_PCI_CLASS;
    cfg[14] = 0x80u; /* type 0, multi-function */
    put32(cfg, 0x10u, 0xfec00000u);
    put16(cfg, 0x2cu, 0x103cu);
    put16(cfg, 0x2eu, 0x2310u); /* synthetic, not HP confirmation */
    cfg[0x3cu] = 11u;
    cfg[0x3du] = 1u;
}
static void regs_fixture(uint8_t regs[96])
{
    memset(regs, 0, 96u);
    put16(regs, 0, 0x2211u); /* 2 output, 2 input, 2 bidir, 64-bit. */
    regs[2] = 0u;
    regs[3] = 1u;
    put32(regs, 8, 1u); /* CRST observed set, no writes. */
    put16(regs, 0x0e, 0x0005u); /* state change bitmask, NOT codec ID. */
    regs[0x4eu] = 0x42u; /* 256 CORB entries advertised; selected=2. */
    regs[0x5eu] = 0x72u; /* 2/16/256 RIRB advertised. */
}
int main(void)
{
    uint8_t cfg[64], regs[96];
    struct whp_hda_pci p;
    struct whp_hda_regs v;
    int checks = 0;
    pci_fixture(cfg);
    memset(&p, 0x5a, sizeof(p));
    CHECK(whp_hda_decode_pci(0, 64, &p) == WHP_HDA_PCI_BAD_INPUT);
    CHECK(whp_hda_decode_pci(cfg, 64, 0) == WHP_HDA_PCI_BAD_INPUT);
    CHECK(whp_hda_decode_pci(cfg, 63, &p) == WHP_HDA_PCI_BAD_INPUT);
    CHECK(p.vendor == 0x5a5au);
    CHECK(whp_hda_decode_pci(cfg, 64, &p) == WHP_HDA_PCI_AMD_FCH_CANDIDATE);
    CHECK(p.vendor == 0x1022u && p.device == 0x780du);
    CHECK(p.command == 6u && p.pci_revision == 1u);
    CHECK(p.multifunction && p.subsystem_vendor == 0x103cu);
    CHECK(p.subsystem_device == 0x2310u && p.irq_line == 11u && p.irq_pin == 1u);
    CHECK(p.bar_kind == WHP_HDA_BAR_MMIO32 && p.mmio_base == UINT64_C(0xfec00000));
    put16(cfg, 0, 0xffffu);
    CHECK(whp_hda_decode_pci(cfg, 64, &p) == WHP_HDA_PCI_NO_DEVICE);
    pci_fixture(cfg);
    cfg[14] = 0x81u;
    CHECK(whp_hda_decode_pci(cfg, 64, &p) == WHP_HDA_PCI_NOT_HDA);
    pci_fixture(cfg);
    cfg[10] = 1u; /* legacy AC'97-like audio class, NOT HDA */
    CHECK(whp_hda_decode_pci(cfg, 64, &p) == WHP_HDA_PCI_NOT_HDA);
    pci_fixture(cfg);
    cfg[9] = 0x80u; /* unsupported HDA prog-if */
    CHECK(whp_hda_decode_pci(cfg, 64, &p) == WHP_HDA_PCI_NOT_HDA);
    pci_fixture(cfg);
    put16(cfg, 2, 0xabcd);
    CHECK(whp_hda_decode_pci(cfg, 64, &p) == WHP_HDA_PCI_CLASS_CANDIDATE);
    pci_fixture(cfg);
    put32(cfg, 0x10, 0x00100004u);
    put32(cfg, 0x14, 1u);
    CHECK(whp_hda_decode_pci(cfg, 64, &p) == WHP_HDA_PCI_AMD_FCH_CANDIDATE);
    CHECK(p.bar_kind == WHP_HDA_BAR_MMIO64);
    CHECK(p.mmio_base == UINT64_C(0x100100000));
    pci_fixture(cfg);
    put32(cfg, 0x10, 0x0000c001u);
    CHECK(whp_hda_decode_pci(cfg, 64, &p) == WHP_HDA_PCI_AMD_FCH_CANDIDATE);
    CHECK(p.bar_kind == WHP_HDA_BAR_NOT_MEMORY && p.mmio_base == 0);
    pci_fixture(cfg);
    put32(cfg, 0x10, 0x00000002u);
    CHECK(whp_hda_decode_pci(cfg, 64, &p) == WHP_HDA_PCI_AMD_FCH_CANDIDATE);
    CHECK(p.bar_kind == WHP_HDA_BAR_INVALID);
    pci_fixture(cfg);
    put32(cfg, 0x10, 0x00000000u);
    CHECK(whp_hda_decode_pci(cfg, 64, &p) == WHP_HDA_PCI_AMD_FCH_CANDIDATE);
    CHECK(p.bar_kind == WHP_HDA_BAR_UNASSIGNED);
    pci_fixture(cfg);
    put32(cfg, 0x10, 0xffffffffu);
    CHECK(whp_hda_decode_pci(cfg, 64, &p) == WHP_HDA_PCI_AMD_FCH_CANDIDATE);
    CHECK(p.bar_kind == WHP_HDA_BAR_UNASSIGNED);

    regs_fixture(regs);
    memset(&v, 0x5a, sizeof(v));
    CHECK(whp_hda_decode_regs(0, 96, &v) == WHP_HDA_REGS_BAD_INPUT);
    CHECK(whp_hda_decode_regs(regs, 96, 0) == WHP_HDA_REGS_BAD_INPUT);
    CHECK(whp_hda_decode_regs(regs, 95, &v) == WHP_HDA_REGS_BAD_INPUT);
    CHECK(v.major == 0x5au);
    CHECK(whp_hda_decode_regs(regs, 96, &v) == WHP_HDA_REGS_OK);
    CHECK(v.major == 1u && v.minor == 0u && v.gcap == 0x2211u);
    CHECK(v.output_streams == 2u && v.input_streams == 2u);
    CHECK(v.bidir_streams == 2u && v.serial_data_outputs == 1u);
    CHECK(v.supports_64bit == 1u && v.gctl == 1u);
    CHECK(v.statests == 5u);
    CHECK(v.corb_supported_sizes == 4u && v.rirb_supported_sizes == 7u);
    CHECK((regs[0x4eu] & 3u) == 2u); /* selected CORB != supported sizes */
    regs[3] = 0u;
    CHECK(whp_hda_decode_regs(regs, 96, &v) == WHP_HDA_REGS_UNSUPPORTED_VERSION);
    CHECK(v.major == 1u); /* unchanged on unsupported version */
    regs_fixture(regs);
    put16(regs, 0, 0u);
    CHECK(whp_hda_decode_regs(regs, 96, &v) == WHP_HDA_REGS_OK);
    CHECK(v.output_streams == 0u && v.input_streams == 0u);
    CHECK(v.supports_64bit == 0u && v.serial_data_outputs == 1u);
    printf("HDA PCI and register probes: %d checks passed\n", checks);
    return 0;
}
