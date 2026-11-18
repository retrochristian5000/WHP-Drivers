/* Portable contract probe: Water-owned Win95 HDA numeric header must
 * agree with the standalone HDA research core. No hardware operations.
 * Unlike the Win16 .DRV probe, this works on a native host compiler.
 */
#include <ddk/win95rtm/audio/hda_regs.h>
#include "hda_probe.h"
#include <stdint.h>

#if WHP_WIN95_HDA_PCI_BASE_CLASS != WHP_HDA_PCI_CLASS
#error "Water HDA PCI base class drift"
#endif
#if WHP_WIN95_HDA_PCI_SUBCLASS != WHP_HDA_PCI_SUBCLASS
#error "Water HDA PCI subclass drift"
#endif
#if WHP_WIN95_HDA_PCI_PROGIF != WHP_HDA_PCI_PROGIF
#error "Water HDA PCI programming-interface drift"
#endif
#if WHP_WIN95_HDA_REG_GCAP != 0x00u || WHP_WIN95_HDA_REG_VMIN != 0x02u || \
    WHP_WIN95_HDA_REG_VMAJ != 0x03u || WHP_WIN95_HDA_REG_GCTL != 0x08u || \
    WHP_WIN95_HDA_REG_STATESTS != 0x0eu
#error "Water HDA global register offset drift"
#endif
#if WHP_WIN95_HDA_REG_CORBSIZE != 0x4eu || \
    WHP_WIN95_HDA_REG_RIRBSIZE != 0x5eu || \
    WHP_WIN95_HDA_REG_RIRBWP != 0x58u
#error "Water HDA ring register offset drift"
#endif
#if WHP_WIN95_HDA_GCAP_NSDO_1 != 0 || \
    WHP_WIN95_HDA_GCAP_NSDO_2 != 2 || \
    WHP_WIN95_HDA_GCAP_NSDO_4 != 4 || \
    WHP_WIN95_HDA_GCAP_NSDO_RESERVED != 6
#error "HDA serial-data-output encoding drift"
#endif
#if WHP_WIN95_HDA_RING_CAP_2 != 0x10u || \
    WHP_WIN95_HDA_RING_CAP_16 != 0x20u || \
    WHP_WIN95_HDA_RING_CAP_256 != 0x40u
#error "HDA CORB/RIRB capability encoding drift"
#endif

int whp_hda_water_header_contract(void)
{
    uint8_t regs[WHP_HDA_MMIO_BYTES] = {0};
    struct whp_hda_regs parsed;
    regs[WHP_WIN95_HDA_REG_VMAJ] = 1u;
    regs[WHP_WIN95_HDA_REG_CORBSIZE] = WHP_WIN95_HDA_RING_CAP_256;
    regs[WHP_WIN95_HDA_REG_RIRBSIZE] = WHP_WIN95_HDA_RING_CAP_16;
    if (whp_hda_decode_regs(regs, sizeof(regs), &parsed) != WHP_HDA_REGS_OK)
        return 1;
    return parsed.corb_supported_sizes == 4u &&
           parsed.rirb_supported_sizes == 2u ? 0 : 2;
}

#ifdef WHP_HDA_WATER_STANDALONE
int main(void)
{
    return whp_hda_water_header_contract();
}
#endif
