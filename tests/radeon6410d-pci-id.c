/* Host-only identity tests. Does not enumerate or access real hardware. */
#include "radeon_hd_6410d.h"

int main(void)
{
    struct whp_r6410d_pci_id pci = {
        WHP_R6410D_VENDOR_AMD,
        WHP_R6410D_DEVICE_DESKTOP,
        WHP_R6410D_PCI_CLASS_DISPLAY,
        WHP_R6410D_PCI_SUBCLASS_VGA
    };

    if (whp_r6410d_identify(&pci) != WHP_R6410D_DESKTOP_CANDIDATE)
        return 1;
    pci.device = WHP_R6410D_DEVICE_MOBILITY;
    if (whp_r6410d_identify(&pci) != WHP_R6410D_MOBILE_CANDIDATE)
        return 2;
    pci.device = 0x9640u; /* Other Radeon Sumo: must NOT match. */
    if (whp_r6410d_identify(&pci) != WHP_R6410D_NO_MATCH)
        return 3;
    pci.device = WHP_R6410D_DEVICE_DESKTOP;
    pci.vendor = 0x1022u; /* AMD CPU/chipset vendor ID is NOT AMD/ATI. */
    if (whp_r6410d_identify(&pci) != WHP_R6410D_NO_MATCH)
        return 4;
    pci.vendor = WHP_R6410D_VENDOR_AMD;
    pci.class_code = 0x02u; /* Network adapter must NOT match. */
    if (whp_r6410d_identify(&pci) != WHP_R6410D_NO_MATCH)
        return 5;
    pci.class_code = WHP_R6410D_PCI_CLASS_DISPLAY;
    pci.subclass = 0x80u; /* Other display subclass: not yet verified. */
    if (whp_r6410d_identify(&pci) != WHP_R6410D_NO_MATCH)
        return 6;
    if (whp_r6410d_identify(0) != WHP_R6410D_NO_MATCH)
        return 7;

    return 0;
}
