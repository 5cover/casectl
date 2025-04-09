// gcc -Wall -Wextra casectl.c -o casectl

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <linux/limits.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#define HELP "Usage: " PROG " [--lower|--upper] [FILE...]\n\
\n\
Encode/decode ALLCAPS files with case-preserving escapes.\n\
Reads from the provided FILEs (\"-\" for stdin) writes to stdout. Reads from stdin if no FILE is provided.\n\
\n\
Options:\n\
  -l, --lower\n\
  \tEncode ALLCAPS to lowercase with escape sequences\n\
  -u, --upper\n\
  \tDecode escaped lowercase back to ALLCAPS\n\
  -i[SUFFIX], --in-place[=SUFFIX]\n\
  \tedit files in place (makes backup if SUFFIX supplied)\n\
  -h, --help\n\
  \tShow this help message and exit\n\
  --version\n\
  \tShow program version and exit\n\
\n\
Defaults to --lower if input is piped. See `man casectl` for details."

/// @brief Versionstring
#define VERSION "1.0.1"

#define MSG_ERROR_ERRNO(ARG) PROG ": error: " ARG ": %s\n"

#define MSG_WARNING PROG ": warning: "

typedef enum {
    RES_OK,
    RES_WARNING,
    RES_ERROR,
} TransformResult;

typedef TransformResult (*transform_t)(FILE *, FILE *);

static TransformResult lowercase(FILE *in, FILE *out);
static TransformResult uppercase(FILE *in, FILE *out);
static int copy_file(const char *src_path, const char *dst_path);
static TransformResult transform_in_place(char const *filename, transform_t transform, char const *in_place_suffix);
static TransformResult transform_regular(char const *filename, transform_t transform);

static bool check_transform_arg_once(transform_t transform) {
    if (transform == NULL) return true;
    fprintf(stderr, PROG ": expected either --lower or --upper, once\n");
    return false;
}

int main(int argc, char **argv) {
    setlocale(LC_CTYPE, "");

    transform_t transform = NULL;
    char const *in_place_suffix = NULL;

    // Arguments
    {
        enum {
            OptHelp = 'h',
            OptInPlace = 'i',
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
                .name = "in-place",
                .val = OptInPlace,
                .has_arg = optional_argument,
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

        while (-1 != (opt = getopt_long(argc, argv, "hi::lu", long_options, NULL))) {
            switch (opt) {
            case OptHelp:
                puts(HELP);
                return EX_OK;
            case OptInPlace:
                in_place_suffix = optarg == NULL ? "" : optarg;
                break;
            case OptVersion:
                puts(VERSION);
                return EX_OK;
            case OptLower:
                if (!check_transform_arg_once(transform)) return EX_USAGE;
                transform = lowercase;
                break;
            case OptUpper:
                if (!check_transform_arg_once(transform)) return EX_USAGE;
                transform = uppercase;
                break;
            case '?':
                puts(HELP);
                return EX_USAGE;
            }
        }
    }

    if (transform == NULL) {
        if (optind >= argc && isatty(STDIN_FILENO)) {
            puts(HELP);
            return EX_OK;
        }
        transform = lowercase;
    }

    if (optind >= argc) return transform(stdin, stdout);

    TransformResult result = RES_OK;
    for (; optind < argc; ++optind) {
        char const *filename = argv[optind];

        TransformResult transform_result = in_place_suffix
                                             ? transform_in_place(filename, transform, in_place_suffix)
                                             : transform_regular(filename, transform);

        if (result < transform_result) {
            result = transform_result;
        }
    }

    return result;
}

TransformResult transform_in_place(char const *filename, transform_t transform, char const *in_place_suffix) {

    if (*in_place_suffix != '\0') { // if suffix provided
        char *filename_backup = malloc(strlen(filename) + strlen(in_place_suffix) + 1);
        sprintf(filename_backup, "%s%s", filename, in_place_suffix);
        bool fail = -1 == copy_file(filename, filename_backup);
        free(filename_backup);
        if (fail) return RES_ERROR;
    }

    // open tmp output
    char tmp_filename[] = "XXXXXX";
    int tmp_fd = mkstemp(tmp_filename);
    if (-1 == tmp_fd) {
        fprintf(stderr, MSG_ERROR_ERRNO("mkstemp"), strerror(errno));
        return RES_ERROR;
    }
    FILE *tmp = fdopen(tmp_fd, "w");
    if (!tmp) {
        fprintf(stderr, MSG_ERROR_ERRNO("fdopen: %s"), tmp_filename, strerror(errno));
        return RES_ERROR;
    }

    // open input
    FILE *in = fopen(filename, "r");
    if (!in) {
        fprintf(stderr, MSG_ERROR_ERRNO("%s"), filename, strerror(errno));
        return RES_ERROR;
    }

    int result = transform(in, tmp);

    fclose(in);

    if (-1 == rename(tmp_filename, filename)) {
        fprintf(stderr, MSG_ERROR_ERRNO("rename: '%s' to '%s'"), tmp_filename, filename, strerror(errno));
        return RES_ERROR;
    }

    return result;
}

TransformResult transform_regular(char const *filename, transform_t transform) {
    // open input
    FILE *in = fopen(filename, "r");
    if (!in) {
        fprintf(stderr, MSG_ERROR_ERRNO("%s"), filename, strerror(errno));
        return RES_ERROR;
    }

    // transform to stdout
    TransformResult result = transform(in, stdout);

    fclose(in);

    return result;
}

TransformResult lowercase(FILE *in, FILE *out) {
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
            return RES_OK;
        default:
            EMIT(c);
            THEN(START);
        }
    case EN: THEN(SPAN_START);
    // EOF must be handled explicitly when the default case isn't an inclusion, to avoid
    case WEOF: return RES_OK;
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
        fprintf(stderr, MSG_WARNING "found an unterminated literal span starting from the last %lc in input. Did you forget to escape it?\n", EN);
        return RES_WARNING;
    default:
        EMIT(c);
        THEN(SPAN_BODY);
    }
}

TransformResult uppercase(FILE *in, FILE *out) {
    wint_t c = READ();

START:
    switch (c) {
    case EN:
        EMIT(EN);
        EMIT(EN);
        THEN(START);
    case WEOF: return RES_OK;
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
        if (next == WEOF) return RES_OK;
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
        return RES_OK;
    default:
        if (iswlower(c)) {
            EMIT(EN);
            INCL(EMIT_UPPER);
        }
        EMIT(c);
        THEN(SPAN_BODY);
    }
}

int copy_file(const char *src_path, const char *dst_path) {
    FILE *src = fopen(src_path, "rb");
    if (!src) {
        fprintf(stderr, MSG_ERROR_ERRNO("%s"), src_path, strerror(errno));
        return -1;
    }

    FILE *dst = fopen(dst_path, "wb");
    if (!dst) {
        fprintf(stderr, MSG_ERROR_ERRNO("%s"), dst_path, strerror(errno));
        fclose(src);
        return -1;
    }

    char buffer[BUFSIZ];
    size_t n;

    while ((n = fread(buffer, 1, sizeof buffer, src)) > 0) {
        if (fwrite(buffer, 1, n, dst) != n) {
            fprintf(stderr, MSG_ERROR_ERRNO("fwrite(%s)"), dst_path, strerror(errno));
            fclose(src);
            fclose(dst);
            return -1;
        }
    }

    if (ferror(src)) {
        fprintf(stderr, MSG_ERROR_ERRNO("fread(%s)"), dst_path, strerror(errno));
        fclose(src);
        fclose(dst);
        return -1;
    }

    fclose(src);
    fclose(dst);
    return 0;
}
