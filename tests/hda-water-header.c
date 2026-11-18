/* Portable contract probe: Water-owned Win95 HDA numeric header must
 * agree with the standalone HDA research core. No hardware operations.
 * Unlike the Win16 .DRV probe, this works on a native host compiler.
 */
#include <ddk/hda/hda_regs.h>
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

#if WHP_WIN95_HDA_PCI_BASE_CLASS != WHP_HDA_PCI_BASE_CLASS
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_PCI_BASE_CLASS"
#endif
#if WHP_WIN95_HDA_PCI_SUBCLASS != WHP_HDA_PCI_SUBCLASS
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_PCI_SUBCLASS"
#endif
#if WHP_WIN95_HDA_PCI_PROGIF != WHP_HDA_PCI_PROGIF
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_PCI_PROGIF"
#endif
#if WHP_WIN95_HDA_REG_GCAP != WHP_HDA_REG_GCAP
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_REG_GCAP"
#endif
#if WHP_WIN95_HDA_REG_VMIN != WHP_HDA_REG_VMIN
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_REG_VMIN"
#endif
#if WHP_WIN95_HDA_REG_VMAJ != WHP_HDA_REG_VMAJ
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_REG_VMAJ"
#endif
#if WHP_WIN95_HDA_REG_GCTL != WHP_HDA_REG_GCTL
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_REG_GCTL"
#endif
#if WHP_WIN95_HDA_REG_STATESTS != WHP_HDA_REG_STATESTS
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_REG_STATESTS"
#endif
#if WHP_WIN95_HDA_REG_INTCTL != WHP_HDA_REG_INTCTL
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_REG_INTCTL"
#endif
#if WHP_WIN95_HDA_REG_INTSTS != WHP_HDA_REG_INTSTS
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_REG_INTSTS"
#endif
#if WHP_WIN95_HDA_REG_CORBLBASE != WHP_HDA_REG_CORBLBASE
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_REG_CORBLBASE"
#endif
#if WHP_WIN95_HDA_REG_CORBUBASE != WHP_HDA_REG_CORBUBASE
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_REG_CORBUBASE"
#endif
#if WHP_WIN95_HDA_REG_CORBWP != WHP_HDA_REG_CORBWP
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_REG_CORBWP"
#endif
#if WHP_WIN95_HDA_REG_CORBRP != WHP_HDA_REG_CORBRP
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_REG_CORBRP"
#endif
#if WHP_WIN95_HDA_REG_CORBCTL != WHP_HDA_REG_CORBCTL
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_REG_CORBCTL"
#endif
#if WHP_WIN95_HDA_REG_CORBSTS != WHP_HDA_REG_CORBSTS
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_REG_CORBSTS"
#endif
#if WHP_WIN95_HDA_REG_CORBSIZE != WHP_HDA_REG_CORBSIZE
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_REG_CORBSIZE"
#endif
#if WHP_WIN95_HDA_REG_RIRBLBASE != WHP_HDA_REG_RIRBLBASE
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_REG_RIRBLBASE"
#endif
#if WHP_WIN95_HDA_REG_RIRBUBASE != WHP_HDA_REG_RIRBUBASE
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_REG_RIRBUBASE"
#endif
#if WHP_WIN95_HDA_REG_RIRBWP != WHP_HDA_REG_RIRBWP
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_REG_RIRBWP"
#endif
#if WHP_WIN95_HDA_REG_RINTCNT != WHP_HDA_REG_RINTCNT
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_REG_RINTCNT"
#endif
#if WHP_WIN95_HDA_REG_RIRBCTL != WHP_HDA_REG_RIRBCTL
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_REG_RIRBCTL"
#endif
#if WHP_WIN95_HDA_REG_RIRBSTS != WHP_HDA_REG_RIRBSTS
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_REG_RIRBSTS"
#endif
#if WHP_WIN95_HDA_REG_RIRBSIZE != WHP_HDA_REG_RIRBSIZE
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_REG_RIRBSIZE"
#endif
#if WHP_WIN95_HDA_GCAP_64OK != WHP_HDA_GCAP_64OK
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_GCAP_64OK"
#endif
#if WHP_WIN95_HDA_GCAP_NSDO_MASK != WHP_HDA_GCAP_NSDO_MASK
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_GCAP_NSDO_MASK"
#endif
#if WHP_WIN95_HDA_GCAP_NSDO_1 != WHP_HDA_GCAP_NSDO_1
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_GCAP_NSDO_1"
#endif
#if WHP_WIN95_HDA_GCAP_NSDO_2 != WHP_HDA_GCAP_NSDO_2
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_GCAP_NSDO_2"
#endif
#if WHP_WIN95_HDA_GCAP_NSDO_4 != WHP_HDA_GCAP_NSDO_4
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_GCAP_NSDO_4"
#endif
#if WHP_WIN95_HDA_GCAP_NSDO_RESERVED != WHP_HDA_GCAP_NSDO_RESERVED
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_GCAP_NSDO_RESERVED"
#endif
#if WHP_WIN95_HDA_GCAP_BSS_MASK != WHP_HDA_GCAP_BSS_MASK
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_GCAP_BSS_MASK"
#endif
#if WHP_WIN95_HDA_GCAP_ISS_MASK != WHP_HDA_GCAP_ISS_MASK
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_GCAP_ISS_MASK"
#endif
#if WHP_WIN95_HDA_GCAP_OSS_MASK != WHP_HDA_GCAP_OSS_MASK
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_GCAP_OSS_MASK"
#endif
#if WHP_WIN95_HDA_RING_SIZE_SEL_MASK != WHP_HDA_RING_SIZE_SEL_MASK
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_RING_SIZE_SEL_MASK"
#endif
#if WHP_WIN95_HDA_RING_CAP_2 != WHP_HDA_RING_CAP_2
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_RING_CAP_2"
#endif
#if WHP_WIN95_HDA_RING_CAP_16 != WHP_HDA_RING_CAP_16
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_RING_CAP_16"
#endif
#if WHP_WIN95_HDA_RING_CAP_256 != WHP_HDA_RING_CAP_256
#error "Win95 compatibility alias drift: WHP_WIN95_HDA_RING_CAP_256"
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
