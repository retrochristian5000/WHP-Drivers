#include "mfc290c_location.h"

#include <stddef.h>

int whp_mfc290c_parse_location_hint(const char *text, uint32_t *out)
{
    uint32_t value = 0u;
    unsigned int i;

    if (text == NULL || out == NULL || text[0] != '0' ||
        (text[1] != 'x' && text[1] != 'X')) {
        return 0;
    }
    for (i = 2u; i < 10u; ++i) {
        unsigned char digit = (unsigned char)text[i];
        uint32_t nibble;

        if (digit >= '0' && digit <= '9') {
            nibble = (uint32_t)(digit - '0');
        } else if (digit >= 'a' && digit <= 'f') {
            nibble = (uint32_t)(digit - 'a' + 10u);
        } else if (digit >= 'A' && digit <= 'F') {
            nibble = (uint32_t)(digit - 'A' + 10u);
        } else {
            return 0;
        }
        value = (value << 4u) | nibble;
    }
    if (text[10] != '\0') {
        return 0;
    }
    *out = value;
    return 1;
}
