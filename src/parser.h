#ifndef PARSER_H
#define PARSER_H

#include "bool.h"
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

typedef struct command_arg_empty {
    char dummy;
} command_arg_empty_t;

typedef struct command_arg_str {
    char* str;
} command_arg_str_t;

typedef struct command_arg_bool {
    bool_t val;
} command_arg_bool_t;

typedef struct command_arg_two_int {
    int i;
    int j;
} command_arg_two_int_t;

typedef struct command_arg_three_int {
    int i;
    int j;
    int k;
} command_arg_three_int_t;

typedef union {
    command_arg_empty_t empty;
    command_arg_str_t str;
    command_arg_bool_t bool;
    command_arg_two_int_t twointegers;
    command_arg_three_int_t threeintegers;
} command_arg_t;

typedef struct command {
    command_type_t type;
    command_arg_t arg;
} command_t;

command_t parse_line(FILE* stream);

#endif
