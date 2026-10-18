/* Compile-only mini-VDD header/ABI checks. No VDD calls, no PCI I/O. */
#include <minivdd.h>
#include <whp/win95rtm/display/minivdd.h>

typedef char whp_check_vdd_base[(MINIVDD_SVC_BASE_OFFSET == 0x80) ? 1 : -1];
typedef char whp_check_vdd_register[(VDD_DRIVER_REGISTER == 0x80) ? 1 : -1];
typedef char whp_check_vdd_unregister[(VDD_DRIVER_UNREGISTER == 0x81) ? 1 : -1];
typedef char whp_check_vdd_config[(VDD_GET_DISPLAY_CONFIG == 0x85) ? 1 : -1];
typedef char whp_check_vdd_mode[(VDD_POST_MODE_CHANGE == 0x87) ? 1 : -1];
typedef char whp_check_mini_chip[(GET_CHIP_ID == 42) ? 1 : -1];
typedef char whp_check_legacy_displayinfo[(sizeof(DISPLAYINFO) == 50) ? 1 : -1];

unsigned int whp_minivdd_header_check(void)
{
    return (unsigned int)(VDD_GET_DISPLAY_CONFIG + GET_CHIP_ID);
}
