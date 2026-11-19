#ifndef WHP_DOS_PROBE_MENU_H
#define WHP_DOS_PROBE_MENU_H
#include <stddef.h>
#include <stdint.h>
#define WHP_PROBE_KEY_UP 0x148
#define WHP_PROBE_KEY_DOWN 0x150
enum whp_probe_menu_action { WHP_MENU_STAY, WHP_MENU_OPEN, WHP_MENU_EXIT };
enum whp_probe_menu_action whp_probe_menu_key(unsigned int *selected, int key);
const char *whp_probe_menu_label(unsigned int section);
int whp_probe_menu_matches(unsigned int section, const uint8_t *cfg, size_t len);
#endif
