/**
 * parser.h - Command parser.
 */

#ifndef PARSER_H
#define PARSER_H

#include "bool.h"
#include <game.h>
#include <stdio.h>

typedef enum command_type {
    CT_SOLVE,
    CT_EDIT,
    CT_MARK_ERRORS,
    CT_PRINT_BOARD,
    CT_SET,
    CT_VALIDATE,
    CT_GUESS,
    CT_GENERATE,
    CT_UNDO,
    CT_REDO,
    CT_SAVE,
    CT_HINT,
    CT_GUESS_HINT,
    CT_NUM_SOLUTIONS,
    CT_AUTOFILL,
    CT_RESET,
    CT_EXIT
} command_type_t;

/**
 * Argument payload representing command with no arguments (print_board,
 * validate, undo, redo, num_solutions, autofill, reset, exit).
 */
typedef struct command_arg_empty {
    char dummy; /* Appease -Wgnu-empty-struct */
} command_arg_empty_t;

/**
 * Argument payload representing commands with a string argument (solve, edit,
 * save).
 */
typedef struct command_arg_str {
    char* str;
} command_arg_str_t;

/**
 * Argument payload representing commands with a boolean argument (mark_errors).
 */
typedef struct command_arg_bool {
    bool_t val;
} command_arg_bool_t;

/**
 * Argument payload representing commands with a floating-point argument
 * (guess).
 */
typedef struct command_arg_float {
    float val;
} command_arg_float_t;

/**
 * Argument payload representing commands with two integer arguments (generate,
 * hint, guess_hint).
 */
typedef struct command_arg_two_int {
    int i;
    int j;
} command_arg_two_int_t;

/**
 * Argument payload representing commands with three integer arguments (set)
 */
typedef struct command_arg_three_int {
    int i;
    int j;
    int k;
} command_arg_three_int_t;

typedef enum parser_error_codes {
    P_SUCCESS,       /* Parsing succeeded */
    P_IO,            /* Unknown IO error */
    P_LINE_TOO_LONG, /* Line length exceeded 256 characters */
    P_IGNORE,        /* Line was blank and should be ignored */
    P_INVALID_MODE,  /* The parsed command was invalid in the specified mode */
    P_INVALID_COMMAND_NAME, /* Unknown command name */
    P_INVALID_NUM_OF_ARGS,  /* Wrong number of arguments to command */
    P_INVALID_ARGUMENTS     /* Invalid argument format for command */
} parser_error_codes_t;

/**
 * Command argument payload.
 * The active member of this union depends on the command parsed.
 */
typedef union {
    command_arg_empty_t empty;
    command_arg_str_t str;
    command_arg_bool_t bool;
    command_arg_float_t onefloat;
    command_arg_two_int_t twointegers;
    command_arg_three_int_t threeintegers;
} command_arg_t;

/**
 * Represents a parsed command.
 */
typedef struct command {
    command_type_t type;
    command_arg_t arg;
} command_t;

/**
 * Destroy a command string argument payload, freeing any allocated memory.
 */
void command_arg_str_destroy(command_arg_str_t* arg);

/**
 * Attempt to parse the next line from `stream` into `cmd` in the specified
 * mode.
 * If parsing fails with one of `P_INVALID_MODE`, `P_INVALID_NUM_OF_ARGS` or
 * `P_INVALID_ARGUMENTS`, the command type is filled out to aid descriptive
 * error messages, but the corresponding argument payload is invalid and should
 * not be used.
 * If parsing succeeds and the command contains a string argument, it must be
 * cleaned up with `command_arg_str_destroy`.
 */
parser_error_codes_t parse_line(FILE* stream, command_t* cmd, game_mode_t mode);

#endif
