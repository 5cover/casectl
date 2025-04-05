#pragma once

/// @brief Read THEN input char (wint_t)
#define READ() getwc(in)
/// @brief Emit a character to output
#define EMIT(C)                                        \
    do {                                               \
        assert(C != WEOF && "Attempted to emit WEOF"); \
        putwc(C, out);                                 \
    } while (0)
/// @brief Run another rule on current input.
#define INCL(LABEL) goto LABEL
/// @brief Read next character, and run anothe rule.
#define THEN(LABEL)    \
    do {               \
        c = getwc(in); \
        INCL(LABEL);   \
    } while (0)
