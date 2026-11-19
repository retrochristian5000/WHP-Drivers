/* Pure menu logic: no DOS calls, hardware reads, or mutable global state. */
#include "probe_menu.h"

enum whp_probe_menu_action whp_probe_menu_key(unsigned int *selected, int key)
{
    if (!selected || *selected < 1u || *selected > 9u)
        return WHP_MENU_STAY;
    if (key == 27 || key == '0' || key == 'q' || key == 'Q')
        return WHP_MENU_EXIT;
    if (key >= '1' && key <= '9') {
        *selected = (unsigned int)(key - '0');
        return WHP_MENU_OPEN;
    }
    if (key == WHP_PROBE_KEY_UP)
        *selected = *selected == 1u ? 9u : *selected - 1u;
    else if (key == WHP_PROBE_KEY_DOWN)
        *selected = *selected == 9u ? 1u : *selected + 1u;
    else if (key == 13)
        return WHP_MENU_OPEN;
    return WHP_MENU_STAY;
}

const char *whp_probe_menu_label(unsigned int section)
{
    switch (section) {
    case 1u: return "DOS version and memory";
    case 2u: return "Legacy BIOS and COM/LPT ports";
    case 3u: return "PCI bridges and ISA/LPC";
    case 4u: return "PCI display";
    case 5u: return "PCI network";
    case 6u: return "PCI high-definition audio";
    case 7u: return "PCI storage";
    case 8u: return "PCI USB";
    case 9u: return "All PCI functions";
    default: return "Unknown section";
    }
}

/* Match standard PCI classes in an already-read 64-byte BIOS header.
 * A class match is not proof of an active driver or functional hardware.
 */
int whp_probe_menu_matches(unsigned int section, const uint8_t *cfg, size_t len)
{
    unsigned int vendor, cls, sub, pi, header_type;
    if (!cfg || len < 64u)
        return 0;
    vendor = (unsigned int)cfg[0] | ((unsigned int)cfg[1] << 8);
    if (vendor == 0u || vendor == 0xffffu)
        return 0;
    cls = cfg[11]; sub = cfg[10]; pi = cfg[9];
    header_type = cfg[14] & 0x7fu;
    switch (section) {
    case 3u: return cls == 6u &&
                     ((sub == 4u && header_type == 1u) ||
                      (sub == 1u && header_type == 0u));
    case 4u: return cls == 3u;
    case 5u: return cls == 2u;
    case 6u: return cls == 4u && sub == 3u &&
                     pi == 0u && header_type == 0u;
    case 7u: return cls == 1u;
    case 8u: return cls == 12u && sub == 3u &&
                     pi != 0xfeu && header_type == 0u;
    case 9u: return 1;
    default: return 0;
    }
}
