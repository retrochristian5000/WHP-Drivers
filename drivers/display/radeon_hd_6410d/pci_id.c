#include "radeon_hd_6410d.h"

/* PCI identification stage only. Do not infer OS support or working BARs. */
enum whp_r6410d_identity whp_r6410d_identify(const struct whp_r6410d_pci_id *id)
{
    if (!id || id->vendor != WHP_R6410D_VENDOR_AMD ||
        id->class_code != WHP_R6410D_PCI_CLASS_DISPLAY ||
        id->subclass != WHP_R6410D_PCI_SUBCLASS_VGA)
        return WHP_R6410D_NO_MATCH;

    if (id->device == WHP_R6410D_DEVICE_DESKTOP)
        return WHP_R6410D_DESKTOP_CANDIDATE;

    if (id->device == WHP_R6410D_DEVICE_MOBILITY)
        return WHP_R6410D_MOBILE_CANDIDATE;

    return WHP_R6410D_NO_MATCH;
}
