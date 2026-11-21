/* Conservative CUPS model-label candidate match (no hardware binding). */
#ifndef WHP_BROTHER_MFC290C_MODEL_MATCH_H
#define WHP_BROTHER_MFC290C_MODEL_MATCH_H

/* True only when the CUPS make/model label contains separate ASCII tokens
 * "Brother" and "MFC-290C" (or "MFC290C"). Not an authenticated device ID.
 * Rejects NULL, overlong labels and longer related model suffixes.
 */
int whp_mfc290c_cups_model_candidate(const char *make_and_model);

#endif
