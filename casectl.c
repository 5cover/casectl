// gcc -wall -wextra casectl.c -o casectl

#include <getopt.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <sysexits.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>

/// @brief one-char escape
#define e1 l'_'

/// @brief n-char escape
#define en l''

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

static wint_t peekwc(FILE *in);
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

// After starting `'
static bool literal_span(file *in, file *out) {
    wint_t c = getwc(in);
    if (c != weof) putwc(c, out);
    return c != en;
}

void lowercase(file *in, file *out) {
    wint_t c;
    bool in_escape = false;

    while ((c = getwc(in)) != weof) {
        switch (c) {
        // single escape
        case e1: {
            if (in_escape) {
                putwc(c, out);
                break;
            }
            wint_t next = getwc(in);
            if (iswupper(next)) {
                putwc(next, out);
            } else {
                putwc(e1, out);
                if (next == en) {
                    in_escape ^= literal_span(in, out);
                } else if (next != e1 && next != weof) {
                    putwc(next, out);
                }
            }
            break;
        }
        // literal span
        case en: {
            in_escape ^= literal_span(in, out);
            break;
        }
        default:
            putwc(in_escape ? c : towlower(c), out);
        }
    }

    if (in_escape) {
        fprintf(stderr, prog ": found an unterminated literal span starting form the last %lc in input. did you forget to escape it?\n", en);
    }
}

static void put_upper_escaped(file *in, file *out, wchar_t c) {
    if (c == e1) {
        wint_t next = peekwc(in);
        if (!iswupper(next)) putwc(e1, out);
    } else if (c == en) {
        putwc(en, out);
    }
    putwc(towupper(c), out);
}

static void put_literal_escaped(wchar_t c, file *out) {
    if (c == en) putwc(en, out);
    putwc(c, out);
}

void uppercase(file *in, file *out) {
    wint_t c;
    while ((c = getwc(in)) != weof) {
        if (iswupper(c)) {
            wint_t next = getwc(in);
            if (iswupper(next)) {
                putwc(en, out);
                put_literal_escaped(c, out);
                do {
                    put_literal_escaped(next, out);
                } while ((next = getwc(in)) != weof && (next == e1 || next == en || iswupper(next)));
                putwc(en, out);
            } else {
                putwc(e1, out);
                put_upper_escaped(in, out, c);
            }
            if (next != weof) put_upper_escaped(in, out, next);
        } else {
            put_upper_escaped(in, out, c);
        }
    }
}

wint_t peekwc(file *in) {
    wint_t c = getwc(in);
    if (c != weof) {
        if (ungetwc(c, in) == weof) {
            fprintf(stderr, prog ": error: peekwc: failed to ungetwc()\n");
        }
    }
    return c;
}