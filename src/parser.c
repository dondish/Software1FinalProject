#include "parser.h"

#include "bool.h"
#include "checked_alloc.h"
#include "game.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Allowed mode flags
 */
enum {
    AM_INIT = (1 << GM_INIT),
    AM_EDIT = (1 << GM_EDIT),
    AM_SOLVE = (1 << GM_SOLVE),
    AM_ALL = AM_INIT | AM_EDIT | AM_SOLVE
};

/**
 * Argument payload type.
 */
typedef enum {
    PT_NONE,
    PT_STR,
    PT_OPT_STR,
    PT_BOOL,
    PT_DOUBLE,
    PT_INT2,
    PT_INT3
} command_arg_type_t;

/**
 * Describes a command, including name, allowed modes and argument types.
 */
typedef struct {
    const char* name;
    command_type_t type;
    unsigned allowed_modes;
    command_arg_type_t payload_type;
} command_desc_t;

/**
 * Tokenize `str` on whitespace with `strtok`.
 */
static char* strtok_ws(char* str) { return strtok(str, " \n\t\v"); }

/**
 * Attempt to extract _exactly_ `size` whitespace-separated arguments from the
 * current string being tokenized by `strtok`, storing the results to `args`.
 * Returns whether extraction succeeded.
 */
static bool_t extract_arguments(char** args, size_t size) {
    size_t i = 0;
    char* tmp;
    for (; i < size; i++) {
        if ((tmp = strtok_ws(NULL)) == NULL) {
            return FALSE;
        }
        args[i] = tmp;
    }
    return strtok_ws(NULL) == NULL;
}

/**
 * Copy the contents of `str` into a new heap-allocated string.
 */
static char* duplicate_str(const char* str) {
    char* dup;

    if (!str) {
        return NULL;
    }

    dup = checked_malloc(strlen(str) + 1);
    strcpy(dup, str);
    return dup;
}

/**
 * Attempt to parse `count` integers from `strs`, storing the results to `vals`.
 */
static bool_t parse_ints(char** strs, int* vals, int count) {
    int i;
    for (i = 0; i < count; i++) {
        if (sscanf(strs[i], "%d", &vals[i]) < 1) {
            return FALSE;
        }
    }
    return TRUE;
}

static parser_error_codes_t parse_arg_payload(command_arg_type_t type,
                                              command_arg_t* arg) {
    switch (type) {
    case PT_NONE:
        if (!extract_arguments(NULL, 0)) {
            return P_INVALID_NUM_OF_ARGS;
        }
        break;
    case PT_STR: {
        char* str;
        if (!extract_arguments(&str, 1)) {
            return P_INVALID_NUM_OF_ARGS;
        }
        arg->str_val = duplicate_str(str);
        break;
    }
    case PT_OPT_STR: {
        char* str = strtok_ws(NULL); /* May be null */

        if (strtok_ws(NULL) != NULL) {
            return P_INVALID_NUM_OF_ARGS;
        }
        arg->str_val = duplicate_str(str);
        break;
    }
    case PT_BOOL: {
        char* str_arg;
        int val;

        if (!extract_arguments(&str_arg, 1)) {
            return P_INVALID_NUM_OF_ARGS;
        }

        if (sscanf(str_arg, "%d", &val) < 1 || (val != 0 && val != 1)) {
            return P_INVALID_ARGUMENTS;
        }

        arg->bool_val = (bool_t)val;
        break;
    }
    case PT_DOUBLE: {
        char* str_arg;
        double val;

        if (!extract_arguments(&str_arg, 1)) {
            return P_INVALID_NUM_OF_ARGS;
        }

        if (sscanf(str_arg, "%lf", &val) < 1) {
            return P_INVALID_ARGUMENTS;
        }

        arg->double_val = val;
        break;
    }
    case PT_INT2: {
        char* str_args[2];
        int vals[2];

        if (!extract_arguments(str_args, 2)) {
            return P_INVALID_NUM_OF_ARGS;
        }

        if (!parse_ints(str_args, vals, 2)) {
            return P_INVALID_ARGUMENTS;
        }

        arg->two_int_val.i = vals[0];
        arg->two_int_val.j = vals[1];
        break;
    }
    case PT_INT3: {
        char* str_args[3];
        int vals[3];

        if (!extract_arguments(str_args, 3)) {
            return P_INVALID_NUM_OF_ARGS;
        }

        if (!parse_ints(str_args, vals, 3)) {
            return P_INVALID_ARGUMENTS;
        }

        arg->three_int_val.i = vals[0];
        arg->three_int_val.j = vals[1];
        arg->three_int_val.k = vals[2];
        break;
    }
    }
    return P_SUCCESS;
}

/**
 * Consume the rest of the current line in `stream`, returning `FALSE` on IO
 * error.
 */
static bool_t read_rest_of_line(FILE* stream) {
    char c;

    do {
        c = fgetc(stream);
    } while (c != '\n' && c != EOF);

    return !ferror(stream);
}

parser_error_codes_t parse_line(FILE* stream, command_t* cmd,
                                game_mode_t mode) {
    static const command_desc_t descs[] = {
        {"solve", CT_SOLVE, AM_ALL, PT_STR},
        {"edit", CT_EDIT, AM_ALL, PT_OPT_STR},
        {"mark_errors", CT_MARK_ERRORS, AM_SOLVE, PT_BOOL},
        {"print_board", CT_PRINT_BOARD, AM_EDIT | AM_SOLVE, PT_NONE},
        {"set", CT_SET, AM_EDIT | AM_SOLVE, PT_INT3},
        {"validate", CT_VALIDATE, AM_EDIT | AM_SOLVE, PT_NONE},
        {"guess", CT_GUESS, AM_SOLVE, PT_DOUBLE},
        {"generate", CT_GENERATE, AM_EDIT, PT_INT2},
        {"undo", CT_UNDO, AM_EDIT | AM_SOLVE, PT_NONE},
        {"redo", CT_REDO, AM_EDIT | AM_SOLVE, PT_NONE},
        {"save", CT_SAVE, AM_EDIT | AM_SOLVE, PT_STR},
        {"hint", CT_HINT, AM_SOLVE, PT_INT2},
        {"guess_hint", CT_GUESS_HINT, AM_SOLVE, PT_INT2},
        {"num_solutions", CT_NUM_SOLUTIONS, AM_EDIT | AM_SOLVE, PT_NONE},
        {"autofill", CT_AUTOFILL, AM_SOLVE, PT_NONE},
        {"reset", CT_RESET, AM_EDIT | AM_SOLVE, PT_NONE},
        {"exit", CT_EXIT, AM_ALL, PT_NONE},
    };

    unsigned mode_mask = 1 << mode;

    char line[258] = {0};
    char* name;
    size_t i;

    if (!fgets(line, 258, stream)) {
        if (feof(stream)) {
            cmd->type = CT_EXIT;
            return P_SUCCESS;
        }

        return P_IO;
    }

    if (line[256] != '\0' && line[256] != '\n') {
        return read_rest_of_line(stream) ? P_LINE_TOO_LONG : P_IO;
    }

    name = strtok_ws(line);

    if (!name) {
        return P_IGNORE;
    }

    for (i = 0; i < sizeof(descs) / sizeof(descs[0]); i++) {
        const command_desc_t* desc = &descs[i];

        if (strcmp(name, desc->name)) {
            continue;
        }

        cmd->type = desc->type;

        if (!(desc->allowed_modes & mode_mask)) {
            return P_INVALID_MODE;
        }

        return parse_arg_payload(desc->payload_type, &cmd->arg);
    }

    return P_INVALID_COMMAND_NAME;
}
