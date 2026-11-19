/* Host-only menu and synthetic PCI category tests. */
#include "probe_menu.h"
#include <stdio.h>
#include <string.h>
#define CHECK(expr) do { ++checks; if (!(expr)) { \
    fprintf(stderr, "probe menu failure at %d: %s\n", __LINE__, #expr); \
    return 1; } } while (0)
int main(void)
{
    unsigned int selected = 1u;
    uint8_t cfg[64];
    int checks = 0;
    memset(cfg, 0, sizeof(cfg));
    cfg[0] = 0xecu; cfg[1] = 0x10u;
    CHECK(whp_probe_menu_key(&selected, WHP_PROBE_KEY_UP) == WHP_MENU_STAY &&
          selected == 9u);
    CHECK(whp_probe_menu_key(&selected, WHP_PROBE_KEY_DOWN) == WHP_MENU_STAY &&
          selected == 1u);
    CHECK(whp_probe_menu_key(&selected, '5') == WHP_MENU_OPEN && selected == 5u);
    CHECK(whp_probe_menu_key(&selected, 13) == WHP_MENU_OPEN);
    CHECK(whp_probe_menu_key(&selected, 27) == WHP_MENU_EXIT);
    CHECK(whp_probe_menu_key(0, 13) == WHP_MENU_STAY);
    CHECK(whp_probe_menu_matches(9u, cfg, 64u));
    CHECK(!whp_probe_menu_matches(9u, cfg, 63u));
    cfg[11] = 3u; CHECK(whp_probe_menu_matches(4u, cfg, 64u));
    cfg[11] = 2u; CHECK(whp_probe_menu_matches(5u, cfg, 64u));
    cfg[11] = 1u; CHECK(whp_probe_menu_matches(7u, cfg, 64u));
    cfg[11] = 4u; cfg[10] = 3u;
    CHECK(whp_probe_menu_matches(6u, cfg, 64u));
    cfg[9] = 1u; CHECK(!whp_probe_menu_matches(6u, cfg, 64u));
    cfg[11] = 12u; cfg[9] = 0x20u;
    CHECK(whp_probe_menu_matches(8u, cfg, 64u));
    cfg[9] = 0xfeu; CHECK(!whp_probe_menu_matches(8u, cfg, 64u));
    cfg[11] = 6u; cfg[10] = 4u; cfg[14] = 1u;
    CHECK(whp_probe_menu_matches(3u, cfg, 64u));
    cfg[10] = 1u; cfg[14] = 0u;
    CHECK(whp_probe_menu_matches(3u, cfg, 64u));
    cfg[0] = 0xffu; cfg[1] = 0xffu;
    CHECK(!whp_probe_menu_matches(9u, cfg, 64u));
    printf("Probe menu: %d checks passed\n", checks);
    return 0;
}
