#ifndef PARSER_H
#define PARSER_H

#include "bool.h"
#include <stdio.h>
#include <game.h>

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

typedef struct command_arg_empty {
    char dummy;
} command_arg_empty_t;

typedef struct command_arg_str {
    char* str;
} command_arg_str_t;

void command_arg_str_destroy(command_arg_str_t* arg);

typedef struct command_arg_bool {
    bool_t val;
} command_arg_bool_t;

typedef struct command_arg_float {
    float val;
} command_arg_float_t;

typedef struct command_arg_one_int {
    int i;
} command_arg_one_int_t;

typedef struct command_arg_two_int {
    int i;
    int j;
} command_arg_two_int_t;

typedef struct command_arg_three_int {
    int i;
    int j;
    int k;
} command_arg_three_int_t;

typedef enum parser_error_codes {
    P_SUCCESS,
    P_LINE_TOO_LONG,
    P_IGNORE,
    P_INVALID_MODE,
    P_INVALID_COMMAND_NAME,
    P_INVALID_NUM_OF_ARGS,
    P_INVALID_ARGUMENTS

} parser_error_codes_t;

typedef union {
    command_arg_empty_t empty;
    command_arg_str_t str;
    command_arg_bool_t bool;
    command_arg_float_t onefloat;
    command_arg_one_int_t oneinteger;
    command_arg_two_int_t twointegers;
    command_arg_three_int_t threeintegers;
} command_arg_t;

typedef struct command {
    command_type_t type;
    command_arg_t arg;
} command_t;

int parse_line(FILE* stream, command_t* cmd, game_mode_t mode);

#endif
