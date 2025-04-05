#pragma once

/// @brief read then input char (wint_t)
#define read() getwc(in)
/// @brief emit a character to output
#define emit(c)                                        \
    do {                                               \
        assert(c != weof && "attempted to emit weof"); \
        putwc(c, out);                                 \
    } while (0)
/// @brief run another rule on current input.
#define incl(label) goto label
/// @brief read next character, and run anothe rule.
#define then(label)    \
    do {               \
        c = getwc(in); \
        incl(label);   \
    } while (0)
