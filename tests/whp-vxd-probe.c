/* Structural VxD linking probe, NOT a production driver; do not install. */
#include <whp/win95rtm/vmm.h>

/* Intentionally minimal: this is only a linker/relocation probe.
 * A real VMM control dispatcher must handle EAX messages and carry status.
 */
__attribute__((naked)) void whp_control(void) {
    __asm__ volatile("clc; ret");
}

__attribute__((used)) struct whp_win95_vxd_ddb WHPTEST_DDB = {
    .sdk = WHP_WIN95_DDK_VERSION,
    .name = {'W','H','P','T','E','S','T',' '},
    .init_order = WHP_WIN95_UNDEFINED_INIT_ORDER,
    .control = whp_control,
    .ddb_size = sizeof(struct whp_win95_vxd_ddb)
};
