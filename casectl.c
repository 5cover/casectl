// gcc -Wall -Wextra casectl.c -o casectl

#include <getopt.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <sysexits.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>

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

static void lowercase(FILE *in, FILE *out);
static void uppercase(FILE *in, FILE *out);

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

    transform_t transform
        = TransformNone;

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
    case TransformLower:
        lowercase(stdin, stdout);
        break;
    case TransformUpper:
        uppercase(stdin, stdout);
        break;
    }
}

// After starting `¤'
static bool literal_span(FILE *in, FILE *out) {
    wint_t c = getwc(in);
    if (c != WEOF) putwc(c, out);
    return c != EN;
}

void lowercase(FILE *in, FILE *out) {
    wint_t c;
    bool in_escape = false;
    
    while ((c = getwc(in)) != WEOF) {
        switch (c) {
        // single escape
        case E1: {
            if (in_escape) {
                putwc(c, out);
                break;
            }
            wint_t next = getwc(in);
            if (iswupper(next)) {
                putwc(next, out);
            } else {
                putwc(E1, out);
                if (next == EN) {
                    in_escape ^= literal_span(in, out);
                } else if (next != E1 && next != WEOF) {
                    putwc(next, out);
                }
            }
            break;
        }
        // literal span
        case EN: {
            in_escape ^= literal_span(in, out);
            break;
        }
        default:
            putwc(in_escape ? c : towlower(c), out);
        }
    }

    if (in_escape) {
        fprintf(stderr, PROG ": Found an unterminated literal span starting form the last %lc in input. Did you forget too escape it?\n", EN);
    }
}

static void put_upper_escaped(FILE *in, FILE *out, wchar_t c) {
    if (c == EN || c == E1) {
        wint_t next = (ungetwc(getwc(in), in));
        if (!iswupper(next)) putwc(c, out);
    }
    putwc(towupper(c), out);
}

static void put_literal_escaped(wchar_t c, FILE *out) {
    if (c == EN) putwc(EN, out);
    putwc(c, out);
}

void uppercase(FILE *in, FILE *out) {
    wint_t c;
    while ((c = getwc(in)) != WEOF) {
        if (iswupper(c)) {
            wint_t next = getwc(in);
            if (iswupper(next)) {
                putwc(EN, out);
                put_literal_escaped(c, out);
                do {
                    put_literal_escaped(next, out);
                } while ((next = getwc(in)) != WEOF && (next == E1 || next == EN || iswupper(next)));
                putwc(EN, out);
            } else {
                putwc(E1, out);
                put_upper_escaped(in, out, c);
            }
            if (next != WEOF) put_upper_escaped(in, out, next);
        } else {
            put_upper_escaped(in, out, c);
        }
    }
}
