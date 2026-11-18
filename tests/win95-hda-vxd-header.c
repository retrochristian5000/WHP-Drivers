/* Win95 RTM HDA/VxD boundary: i386 compile-only, NO VxD entry point.
 * Water owns the historical VMM DDB reference and HDA register constants.
 * HDA is newer than retail Win95; this object proves only header fit,
 * not presence of HDA enumeration, runtime services, or a loadable driver.
 */
#include <ddk/win95rtm/vmm.h>
#include <ddk/win95rtm/audio/hda_regs.h>
#include "hda_probe.h"

#if WHP_WIN95_DDK_VERSION != 0x0400u
#error "Wrong Windows 95 RTM VMM reference version"
#endif
#if WHP_WIN95_HDA_REG_GCAP != 0x00u || WHP_WIN95_HDA_REG_GCTL != 0x08u
#error "HDA global-controller register mismatch"
#endif
#if WHP_WIN95_HDA_REG_CORBSIZE != 0x4eu || WHP_WIN95_HDA_REG_RIRBSIZE != 0x5eu
#error "HDA command/response ring offset mismatch"
#endif
_Static_assert(sizeof(struct whp_win95_vxd_ddb) == 80u,
               "Win95 VxD DDB width mismatch");
_Static_assert(sizeof(uint32_t) == 4u,
               "Win95 HDA register reference needs 32-bit words");

unsigned int whp_win95_hda_vxd_header_probe(void)
{
    /* Numeric proof only; does not dereference MMIO or register a VxD. */
    return WHP_WIN95_HDA_REG_CORBSIZE +
           WHP_WIN95_HDA_REG_RIRBSIZE +
           WHP_HDA_MMIO_BYTES;
}
