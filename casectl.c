// gcc -Wall -Wextra casectl.c -o casectl

#include <assert.h>
#include <getopt.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <sysexits.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>

#include "ror.h"

/// @brief One-char escape
#define E1 L'_'

/// @brief N-char escape
#define EN L'¤'

/// @brief Program name
#define PROG "casectl"

/// @brief Helpstring
#define HELP "Usage: " PROG " [--lower|--upper]\n\
\n\
Encode/decode ALLCAPS source files with case-preserving escapes.\n\
Reads from stdin, writes to stdout.\n\
\n\
Options:\n\
  -l, --lower    Encode ALLCAPS to lowercase with escape sequences\n\
  -u, --upper    Decode escaped lowercase back to ALLCAPS\n\
  -h, --help     Show this help message and exit\n\
      --version  Show program version and exit\n\
\n\
Defaults to --lower if input is piped. See `man casectl` for details."

/// @brief Versionstring
#define VERSION "1.0.0"

static int lowercase(FILE *in, FILE *out);
static int uppercase(FILE *in, FILE *out);

typedef enum {
    TransformNone,
    TransformUpper,
    TransformLower,
} transform_t;

static bool check_transform_arg_once(transform_t transform) {
    if (transform == TransformNone) return true;
    fprintf(stderr, PROG ": expected either --lower or --upper, once\n");
    return false;
}

int main(int argc, char **argv) {
    setlocale(LC_CTYPE, "");

    transform_t transform = TransformNone;

    // Arguments
    {
        enum {
            OptHelp = 'h',
            OptLower = 'l',
            OptUpper = 'u',
            OptVersion,
        };
        struct option long_options[] = {
            {
                .name = "help",
                .val = OptHelp,
            },
            {
                .name = "lower",
                .val = OptLower,
            },
            {
                .name = "upper",
                .val = OptUpper,
            },
            {
                .name = "version",
                .val = OptVersion,
            },
            { 0 },
        };

        int opt;

        while (-1 != (opt = getopt_long(argc, argv, "hlu", long_options, NULL))) {
            switch (opt) {
            case OptHelp:
                puts(HELP);
                return EX_OK;
            case OptVersion:
                puts(VERSION);
                return EX_OK;
            case OptLower:
                if (!check_transform_arg_once(transform)) return EX_USAGE;
                transform = TransformLower;
                break;
            case OptUpper:
                if (!check_transform_arg_once(transform)) return EX_USAGE;
                transform = TransformUpper;
                break;
            case '?':
                puts(HELP);
                return EX_USAGE;
            }
        }
    }

    switch (transform) {
    case TransformNone:
        if (isatty(STDIN_FILENO)) {
            puts(HELP);
            break;
        }
        [[fallthrough]];
    case TransformLower: return lowercase(stdin, stdout);
    case TransformUpper: return uppercase(stdin, stdout);
    }
}

int lowercase(FILE *in, FILE *out) {
    wint_t c = READ();

START:
    switch (c) {
    case E1:
        c = READ();
        switch (c) {
        case EN:
            EMIT(E1);
            THEN(SPAN_START);
        case WEOF:
            EMIT(E1);
            return 0;
        default:
            EMIT(c);
            THEN(START);
        }
    case EN: THEN(SPAN_START);
    // EOF must be handled explicitly when the default case isn't an inclusion, to avoid
    case WEOF: return 0;
    default:
        EMIT(towlower(c));
        THEN(START);
    }

SPAN_START:
    switch (c) {
    case EN:
        EMIT(EN);
        THEN(START);
    default: INCL(SPAN_BODY);
    }

SPAN_BODY:
    switch (c) {
    case EN:
        c = READ();
        switch (c) {
        case EN:
            EMIT(EN);
            THEN(SPAN_BODY);
        default: INCL(START);
        }
    case WEOF:
        fprintf(stderr, PROG ": Found an unterminated literal span starting from the last %lc in input. Did you forget to escape it?\n", EN);
        return 1;
    default:
        EMIT(c);
        THEN(SPAN_BODY);
    }
}

int uppercase(FILE *in, FILE *out) {
    wint_t c = READ();

START:
    switch (c) {
    case EN:
        EMIT(EN);
        EMIT(EN);
        THEN(START);
    case WEOF: return 0;
    default:
        // if character does not require escaping
        if (c != E1 && !iswupper(c)) INCL(EMIT_UPPER);

        wint_t next = READ();

        // If the next char would also require _-escaping, start a literal span.
        if (next == E1 || next == EN || iswupper(next)) {
            EMIT(EN);
            EMIT(c);
            c = next;
            INCL(SPAN_BODY);
        }

        // otherwise, just _-escape it normally.
        EMIT(E1);
        EMIT(c);
        if (next == WEOF) return 0;
        EMIT(towupper(next));
        THEN(START);
    }

EMIT_UPPER:
    EMIT(towupper(c));
    THEN(START);

SPAN_BODY:
    switch (c) {
    case EN:
        EMIT(EN);
        EMIT(EN);
        THEN(SPAN_BODY);
    case WEOF:
        EMIT(EN);
        return 0;
    default:
        if (iswlower(c)) {
            EMIT(EN);
            INCL(EMIT_UPPER);
        }
        EMIT(c);
        THEN(SPAN_BODY);
    }
}
