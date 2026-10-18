/* Structural VxD linking probe, NOT a production driver; do not install. */
typedef unsigned int U32;
typedef unsigned short U16;
typedef unsigned char U8;
#pragma pack(push, 1)
struct DDB {
    U32 next;
    U16 sdk, id;
    U8 major, minor;
    U16 flags;
    U8 name[8];
    U32 init_order;
    void (*control)(void);
    U32 v86, pm, v86_csip, pm_csip, ref_data;
    U32 services, services_count, win32_services, prev, ddb_size;
    U32 reserved1, reserved2, reserved3;
};
#pragma pack(pop)
_Static_assert(sizeof(struct DDB) == 80, "VxD DDB must be 80 bytes");

/* Intentionally minimal: this is only a linker/relocation probe. */
__attribute__((naked)) void whp_control(void) {
    __asm__ volatile("clc; ret");
}

__attribute__((used)) struct DDB WHPTEST_DDB = {
    .sdk = 0x0400,
    .name = {'W','H','P','T','E','S','T',' '},
    .init_order = 0x80000000,
    .control = whp_control,
    .ddb_size = sizeof(struct DDB)
};
