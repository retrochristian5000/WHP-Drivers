/* OpenWatcom Windows VxD *format* experiment; NOT a loadable driver.
 * Deliberately uses only pre-C99 declarations supported by old Watcom C.
 * Do not treat this entry as a VMM control dispatcher.
 */
typedef unsigned long U32;
typedef unsigned short U16;
typedef unsigned char U8;
#pragma pack(push, 1)
struct whp_watcom_ddb {
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
typedef char whp_watcom_ddb_size_must_be_80[
    sizeof(struct whp_watcom_ddb) == 80 ? 1 : -1
];
void whp_watcom_control(void) {}
struct whp_watcom_ddb WHPWAT_DDB = {
    0, 0x0400, 0, 0, 0, 0,
    {'W', 'H', 'P', 'W', 'A', 'T', ' ', ' '},
    0x80000000UL, whp_watcom_control
};
