/* MFC-290C macOS USB topology hint: never a device binding key. */
#ifndef WHP_BROTHER_MFC290C_LOCATION_H
#define WHP_BROTHER_MFC290C_LOCATION_H

#include <stdint.h>

/* Parse exactly 0x followed by eight hexadecimal digits. Output is unchanged
 * on invalid input. No decoding or persistent identity is implied.
 */
int whp_mfc290c_parse_location_hint(const char *text, uint32_t *out);

#endif
