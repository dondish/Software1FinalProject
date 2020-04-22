#include "mainaux.h"

#include "board.h"
#include "bool.h"
#include "game.h"
#include "history.h"
#include "lp.h"
#include "parser.h"
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Display a formatted error message to the user.
 */
static void __attribute__((format(printf, 1, 2)))
print_error(const char* fmt, ...) {
    va_list ap;
    printf("Error: ");
    vprintf(fmt, ap);
    puts("");
}

/**
 * Display a formatted success message to the user.
 */
void __attribute__((format(printf, 1, 2))) print_success(const char* fmt, ...) {
    va_list ap;
    vprintf(fmt, ap);
    puts("");
}

bool_t init_game(game_t* game) {
    if (!lp_env_init(&game->lp_env)) {
        print_error("Failed to initialize Gurobi.");
        return FALSE;
    }

    game->mode = GM_INIT;
    /* Placeholder, so that the board is always valid */
    board_init(&game->board, 0, 0);
    history_init(&game->history);
    return TRUE;
}

void destroy_game(game_t* game) {
    lp_env_destroy(&game->lp_env);
    history_destroy(&game->history);
    board_destroy(&game->board);
}

typedef struct command_lookup_cell {
    command_type_t type;
    const char* message;
} command_lookup_cell_t;

static void print_usage(command_type_t type) {
    static const command_lookup_cell_t command_lookup_table[] = {
        {CT_SOLVE, "solve <file path>"},
        {CT_EDIT, "edit [file path]"},
        {CT_MARK_ERRORS, "mark_errors <boolean>"},
        {CT_PRINT_BOARD, "print_board"},
        {CT_SET, "set <column> <row> <value>"},
        {CT_VALIDATE, "validate"},
        {CT_GUESS, "guess <threshold>"},
        {CT_GENERATE, "generate <amount of empty cells> <amount of random "
                      "cells that remain>"},
        {CT_UNDO, "undo"},
        {CT_REDO, "redo"},
        {CT_SAVE, "save <file path>"},
        {CT_HINT, "hint <column> <row>"},
        {CT_GUESS_HINT, "guess_hint <column> <row>"},
        {CT_NUM_SOLUTIONS, "num_solutions"},
        {CT_AUTOFILL, "autofill"},
        {CT_RESET, "reset"},
        {CT_EXIT, "exit"},
    };

    size_t i;
    for (i = 0;
         i < sizeof(command_lookup_table) / sizeof(command_lookup_cell_t);
         i++) {
        if (command_lookup_table[i].type == type) {
            printf("Usage: %s\n", command_lookup_table[i].message);
            return;
        }
    }
}

void print_parser_error(command_t* cmd, parser_error_codes_t error) {
    switch (error) {
    case P_SUCCESS:
    case P_IGNORE:
        break;
    case P_IO:
        print_error("Failed to read from standard input.");
        break;
    case P_LINE_TOO_LONG:
        print_error("Command input exceeded maximum length of 256.");
        break;
    case P_INVALID_MODE:
        print_error("Command is not valid in this mode.");
        break;
    case P_INVALID_COMMAND_NAME:
        print_error("Invalid command.");
        break;
    case P_INVALID_NUM_OF_ARGS:
        print_error("Incorrect number of arguments.");
        print_usage(cmd->type);
        break;
    case P_INVALID_ARGUMENTS:
        print_error("Invalid arguments.");
        print_usage(cmd->type);
        break;
    }
}

static bool_t load_board_from_file(board_t* board, const char* filename) {
    FILE* file;

    file = fopen(filename, "r");

    if (file == NULL) {
        print_error("Failed to open file '%s': %s.", filename, strerror(errno));
        return FALSE;
    }

    switch (board_deserialize(board, file)) {
    case DS_OK:
        return TRUE;
    case DS_ERR_FMT:
        print_error("Invalid file format.");
    case DS_ERR_CELL:
        print_error("Invalid cell encountered.");
    case DS_ERR_IO:
        print_error("Error loading board from file: %s.", strerror(errno));
    }

    return FALSE;
}

static void game_board_print(const game_t* game) {
    board_print(&game->board, stdout,
                game->mark_errors || game->mode == GM_EDIT);
}

bool_t command_execute(game_t* game, command_t* command) {
    switch (command->type) {
    case CT_SOLVE: {
        board_t board, fixed_check;
        int i, j;

        char* filename = command->arg.str_val;

        if (!load_board_from_file(&board, filename)) {
            break;
        }

        board_init(&fixed_check, board.m, board.n);

        for (i = 0; i < board.m * board.n; i++) {
            for (j = 0; j < board.m * board.n; j++) {
                if (!(board_access(&fixed_check, i, j)->flags & CF_FIXED)) {
                    board_access(&fixed_check, i, j)->value = 0;
                }
            }
        }

        if (!board_is_legal(&fixed_check)) {
            print_error("The board's fixed cells are illegaly placed.");
        } else {
            history_clear(&game->history);
            game->mode = GM_SOLVE;
            board_clone(&game->board, &board);
            game_board_print(game);
        }
        free(filename);
        break;
    }
    case CT_EDIT: {
        if (command->arg.str_val) {
            board_t copy;
            if (!load_board_from_file(&copy, command->arg.str_val)) {
                break;
            }

            board_clone(&game->board, &copy);
        } else {
            board_init(&game->board, 3, 3);
        }
        game->mode = GM_EDIT;
        history_clear(&game->history);
        board_mark_errors(&game->board);
        game_board_print(game);
        break;
    }
    case CT_MARK_ERRORS: {
        game->mark_errors = command->arg.bool_val;
        break;
    }
    case CT_PRINT_BOARD: {
        game_board_print(game);
        break;
    }
    case CT_SET: {
        int x = command->arg.three_int_val.i, y = command->arg.three_int_val.j;
        int val = command->arg.three_int_val.k;
        int block_size = board_block_size(&game->board);
        delta_list_t updates;

        if (x <= 0 || x > block_size || y <= 0 || y > block_size) {
            print_error("The indices are out of range.");
            break;
        }

        if (val < 0 || val > block_size) {
            print_error("The value is out of range.");
            break;
        }

        if (cell_is_fixed(board_access(&game->board, y - 1, x - 1))) {
            print_error("This cell is fixed and cannot be updated.");
            break;
        }

        delta_list_init(&updates);
        delta_list_add(&updates, y - 1, x - 1,
                       board_access(&game->board, y - 1, x - 1)->value, val);
        delta_list_apply(&game->board, &updates, NULL, NULL);
        history_add_item(&game->history, &updates);

        board_mark_errors(&game->board);
        game_board_print(game);
        break;
    }
    case CT_VALIDATE: {
        lp_status_t status;
        board_t copy;
        if (!board_is_legal(&game->board)) {
            print_error("The board is erroneous.");
            break;
        }
        board_clone(&copy, &game->board);
        status = lp_solve_ilp(&game->lp_env, &copy);
        if (status == LP_SUCCESS) {
            print_success("The board is solveable.");
        } else if (status == LP_INFEASIBLE) {
            print_success("The board is unsolveable.");
        } else {
            print_error("Gurobi failed to compute the board.");
        }
        board_destroy(&copy);
        break;
    }
    case CT_GUESS: {
        break;
    }
    case CT_EXIT: {
        print_success("Exiting...");
        return FALSE;
    }
    default:
        break;
    }
    return TRUE;
}
