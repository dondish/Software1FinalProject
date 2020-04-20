#include "parser.h"

#include "bool.h"
#include "checked_alloc.h"
#include "game.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void command_arg_str_destroy(command_arg_str_t* arg) { free(arg->str); }

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

parser_error_codes_t parse_line(FILE* stream, command_t* cmd,
                                game_mode_t mode) {
    char line[258] = {0};
    char* curr;

    fgets(line, 258, stream);

    if (ferror(stream)) {
        return P_IO;
    }

    if (line[256] != 0 && line[256] != '\n') {
        return read_rest_of_line(stream) ? P_LINE_TOO_LONG : P_IO;
    }

    curr = strtok_ws(line);

    if (curr == NULL)
        return P_IGNORE;

    if (!strcmp(curr, "solve")) {
        command_arg_str_t arg;
        char* temparg;

        cmd->type = CT_SOLVE;

        if (!extract_arguments(&temparg, 1)) {
            return P_INVALID_NUM_OF_ARGS;
        }

        arg.str = duplicate_str(temparg);
        cmd->arg.str = arg;
    } else if (!strcmp(curr, "edit")) {
        command_arg_str_t arg;

        /* Note: not using extract_arguments as filename is optional. */
        char* temparg = strtok_ws(NULL);

        cmd->type = CT_EDIT;

        arg.str = duplicate_str(temparg);
        cmd->arg.str = arg;
    } else if (!strcmp(curr, "mark_errors")) {
        command_arg_bool_t boolt;
        int x;
        char* temparg;

        cmd->type = CT_MARK_ERRORS;

        if (mode != GM_SOLVE)
            return P_INVALID_MODE;

        if (!extract_arguments(&temparg, 1)) {
            return P_INVALID_NUM_OF_ARGS;
        } else if (!sscanf(temparg, "%d", &x) || x > 1 || x < 0) {
            return P_INVALID_ARGUMENTS;
        } else {
            boolt.val = x;
        }

        cmd->arg.bool = boolt;
    } else if (!strcmp(curr, "print_board")) {
        cmd->type = CT_PRINT_BOARD;
        if (mode == GM_INIT)
            return P_INVALID_MODE;
    } else if (!strcmp(curr, "set")) {
        command_arg_three_int_t arg;
        char* strparams[3];
        int params[3];
        int i;

        cmd->type = CT_SET;

        if (mode == GM_INIT)
            return P_INVALID_MODE;

        if (!extract_arguments(strparams, 3)) {
            return P_INVALID_NUM_OF_ARGS;
        }

        for (i = 0; i < 3; i++) {
            if (!sscanf(strparams[i], "%d", &params[i])) {
                return P_INVALID_ARGUMENTS;
            }
        }

        arg.i = params[0];
        arg.j = params[1];
        arg.k = params[2];
        cmd->arg.threeintegers = arg;
    } else if (!strcmp(curr, "validate")) {
        cmd->type = CT_VALIDATE;
        if (mode == GM_INIT)
            return P_INVALID_MODE;
    } else if (!strcmp(curr, "guess")) {
        command_arg_float_t arg;
        char* temparg;

        cmd->type = CT_GUESS;

        if (mode != GM_SOLVE)
            return P_INVALID_MODE;

        if (!extract_arguments(&temparg, 1)) {
            return P_INVALID_NUM_OF_ARGS;
        }

        if (!sscanf(temparg, "%f", &arg.val)) {
            return P_INVALID_ARGUMENTS;
        }

        cmd->arg.onefloat = arg;
    } else if (!strcmp(curr, "generate")) {
        command_arg_two_int_t arg;
        char* args[2];

        cmd->type = CT_GENERATE;

        if (mode != GM_EDIT)
            return P_INVALID_MODE;

        if (!extract_arguments(args, 2)) {
            return P_INVALID_NUM_OF_ARGS;
        }

        if (!sscanf(args[0], "%d", &arg.i) || arg.i < 0)
            return P_INVALID_ARGUMENTS;
        if (!sscanf(args[1], "%d", &arg.j) || arg.j <= 0)
            return P_INVALID_ARGUMENTS;

        cmd->arg.twointegers = arg;
    } else {
        return P_INVALID_COMMAND_NAME;
    }

    if (strtok_ws(NULL) != NULL) {
        return P_INVALID_NUM_OF_ARGS;
    }

    return P_SUCCESS;
}
