/* Read-only CUPS queue model audit for the macOS Brother probe. */
#ifndef WHP_MFC290C_MACOS_CUPS_H
#define WHP_MFC290C_MACOS_CUPS_H

/* Queries the CUPS scheduler. Does not print, add, or modify queues.
 * Model-label matches are candidates only, not evidence of USB binding.
 */
void whp_mfc290c_report_cups(void);

#endif
