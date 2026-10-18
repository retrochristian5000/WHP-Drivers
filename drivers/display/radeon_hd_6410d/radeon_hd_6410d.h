/* Radeon HD 6410D PCI identity only: no hardware register access.
 * Candidates: PCI 1002:9644 (desktop Sumo2), 1002:9645 (mobile Sumo2).
 * Do not treat either ID as confirmation of the installed adapter.
 */
#ifndef WHP_RADEON_HD_6410D_H
#define WHP_RADEON_HD_6410D_H

#define WHP_R6410D_VENDOR_AMD        0x1002u
#define WHP_R6410D_DEVICE_DESKTOP    0x9644u
#define WHP_R6410D_DEVICE_MOBILITY   0x9645u
#define WHP_R6410D_PCI_CLASS_DISPLAY        0x03u
#define WHP_R6410D_PCI_SUBCLASS_VGA         0x00u

struct whp_r6410d_pci_id {
    unsigned short vendor;
    unsigned short device;
    unsigned char class_code;
    unsigned char subclass;
};

enum whp_r6410d_identity {
    WHP_R6410D_NO_MATCH = 0,
    WHP_R6410D_DESKTOP_CANDIDATE = 1,
    WHP_R6410D_MOBILE_CANDIDATE = 2
};

/* A pure classifier; it does not enumerate PCI, bind a device, or program
 * VRAM/MMIO. The display driver is not operational.
 */
enum whp_r6410d_identity whp_r6410d_identify(const struct whp_r6410d_pci_id *id);

#endif
