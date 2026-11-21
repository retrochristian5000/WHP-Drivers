#include "mfc290c_model_match.h"

#include <stddef.h>

#define WHP_MFC290C_MAX_MODEL_LABEL 256u

static int ascii_letter_or_digit(unsigned char character)
{
    return (character >= 'a' && character <= 'z') ||
           (character >= 'A' && character <= 'Z') ||
           (character >= '0' && character <= '9');
}

static unsigned char ascii_lower(unsigned char character)
{
    if (character >= 'A' && character <= 'Z') {
        return (unsigned char)(character + ('a' - 'A'));
    }
    return character;
}

static int token_present(const char *label, size_t length,
                         const char *token, size_t token_length)
{
    size_t i;
    size_t j;

    for (i = 0u; i + token_length <= length; ++i) {
        if ((i != 0u && ascii_letter_or_digit((unsigned char)label[i - 1u])) ||
            (i + token_length < length &&
             ascii_letter_or_digit((unsigned char)label[i + token_length]))) {
            continue;
        }
        for (j = 0u; j < token_length; ++j) {
            if (ascii_lower((unsigned char)label[i + j]) !=
                (unsigned char)token[j]) {
                break;
            }
        }
        if (j == token_length) {
            return 1;
        }
    }
    return 0;
}

int whp_mfc290c_cups_model_candidate(const char *make_and_model)
{
    size_t length = 0u;

    if (make_and_model == NULL) {
        return 0;
    }
    while (length <= WHP_MFC290C_MAX_MODEL_LABEL &&
           make_and_model[length] != '\0') {
        ++length;
    }
    if (length == 0u || length > WHP_MFC290C_MAX_MODEL_LABEL) {
        return 0;
    }

    return token_present(make_and_model, length, "brother", 7u) &&
           (token_present(make_and_model, length, "mfc-290c", 8u) ||
            token_present(make_and_model, length, "mfc290c", 7u));
}
