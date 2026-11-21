/* CUPS queue metadata is a separate source from USB IORegistry evidence. */
#include <cups/cups.h>

#include <stdio.h>

#include "mfc290c_cups.h"
#include "mfc290c_model_match.h"

void whp_mfc290c_report_cups(void)
{
    cups_dest_t *destinations = NULL;
    int count;
    int i;
    unsigned int model_candidates = 0u;
    unsigned int missing_model_labels = 0u;

    count = cupsGetDests(&destinations);
    if (count < 0 || (count > 0 && destinations == NULL)) {
        puts("CUPS destination inventory unavailable; no printer conclusion.");
        return;
    }

    for (i = 0; i < count; ++i) {
        const char *label = cupsGetOption(
            "printer-make-and-model",
            destinations[i].num_options,
            destinations[i].options);

        if (label == NULL) {
            ++missing_model_labels;
        } else if (whp_mfc290c_cups_model_candidate(label)) {
            ++model_candidates;
        }
    }

    printf("CUPS destinations=%d MFC-290C model-label candidates=%u"
           " missing-model-label=%u\n",
           count, model_candidates, missing_model_labels);
    puts("CUPS metadata is unauthenticated: model-label candidates are not USB matches.");
    puts("No print jobs were submitted and no queues were changed.");
    if (count > 0) {
        cupsFreeDests(count, destinations);
    }
}
