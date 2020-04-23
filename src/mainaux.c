/**
 * Main game/TUI implementation. State updates and TUI output are so intertwined
 * that we decided against splitting them into separate modules.
 */

#include "mainaux.h"

#include "backtrack.h"
#include "board.h"
#include "bool.h"
#include "checked_alloc.h"
#include "game.h"
#include "history.h"
#include "lp.h"
#include "parser.h"
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Display a formatted error message to the user.
 */
static void __attribute__((format(printf, 1, 2)))
print_error(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    printf("Error: ");
    vprintf(fmt, ap);
    va_end(ap);
    puts("");
}

/**
 * Display a formatted success message to the user.
 */
void __attribute__((format(printf, 1, 2))) print_success(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    puts("");
}

/* Game Initialization/Destruction */

bool_t init_game(game_t* game) {
    if (!lp_env_create(&game->lp_env)) {
        print_error("Failed to initialize Gurobi.");
        return FALSE;
    }

    game->mode = GM_INIT;
    game->mark_errors = TRUE;

    /* Note: this placeholder can be destroyed via board_destroy without any
     * ill effects. */
    memset(&game->board, 0, sizeof(board_t));
    history_init(&game->history);

    return TRUE;
}

void destroy_game(game_t* game) {
    lp_env_free(game->lp_env);
    history_destroy(&game->history);
    board_destroy(&game->board);
}

/* Prompt Display */

static const char* game_mode_to_str(game_mode_t mode) {
    switch (mode) {
    case GM_INIT:
        return "init";
    case GM_EDIT:
        return "edit";
    case GM_SOLVE:
        return "solve";
    }

    return "";
}

void print_prompt(game_t* game) {
    printf("%s> ", game_mode_to_str(game->mode));
}

/* Parser Error Display */

typedef struct command_lookup_cell {
    command_type_t type;
    const char* message;
} command_lookup_cell_t;

/**
 * Print usage information for the specified command type.
 */
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

/* Command Execution */

/**
 * Check whether errors should currently be marked when printing the game board.
 */
static bool_t should_mark_errors(const game_t* game) {
    return game->mark_errors || game->mode == GM_EDIT;
}

/**
 * Print the current game board.
 */
static void game_board_print(const game_t* game) {
    board_print(&game->board, stdout, should_mark_errors(game));
}

/**
 * Attempt to open `filename` in the specified mode, printing error messages to
 * the user.
 */
static FILE* open_file(const char* filename, const char* mode) {
    FILE* file = fopen(filename, mode);

    if (!file) {
        print_error("Failed to open file '%s': %s.", filename, strerror(errno));
        return FALSE;
    }

    return file;
}

/**
 * Attempt to load `board` from the specified filename, displaying any errors to
 * the user.
 */
static bool_t load_board_from_file(board_t* board, const char* filename) {
    deserialize_status_t status;
    FILE* file = open_file(filename, "r");

    if (!file) {
        return FALSE;
    }

    status = board_deserialize(board, file);
    fclose(file);

    switch (status) {
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

/**
 * Update the game mode to the specified mode, clearing history and replacing
 * the board.
 */
static void enter_game_mode(game_t* game, game_mode_t mode, board_t* board) {
    game->mode = mode;

    history_clear(&game->history);

    board_destroy(&game->board);
    memcpy(&game->board, board, sizeof(board_t));
    memset(board, 0, sizeof(board_t));

    print_success("Entering %s mode...", game_mode_to_str(mode));
}

/**
 * Check whether `board` is full.
 */
static bool_t board_is_full(const board_t* board) {
    int block_size = board_block_size(board);

    int row;
    int col;

    for (row = 0; row < block_size; row++) {
        for (col = 0; col < block_size; col++) {
            if (cell_is_empty(board_access_const(board, row, col))) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

/**
 * Check whether the game board has been solved, printing an appropriate
 * message and switching back to init mode if it has.
 */
static void game_handle_solved(game_t* game) {
    if (!board_is_full(&game->board)) {
        return;
    }

    if (board_is_legal(&game->board)) {
        board_t dummy = {0}; /* Note: board_destroy on this is a no-op */

        print_success("Puzzle solved successfully!");
        enter_game_mode(game, GM_INIT, &dummy);
    } else {
        print_success("Puzzle solution is incorrect.");
    }
}

/**
 * Call this after processing a command that may have changed the board -
 * re-mark and reprint the board, notify the user if they have solved the puzzle
 * in solve mode.
 */
static void game_board_after_change(game_t* game) {
    board_mark_errors(&game->board);
    game_board_print(game);

    if (game->mode == GM_SOLVE) {
        game_handle_solved(game);
    }
}

/**
 * Initialize a new board in `dest` containing only fixed cells from `src`.
 */
static void clone_fixed(board_t* dest, const board_t* src) {
    int block_size = board_block_size(src);

    int row;
    int col;

    board_init(dest, src->m, src->n);

    for (row = 0; row < block_size; row++) {
        for (col = 0; col < block_size; col++) {
            const cell_t* src_cell = board_access_const(src, row, col);

            if (cell_is_fixed(src_cell)) {
                board_access(dest, row, col)->value = src_cell->value;
            }
        }
    }
}

/**
 * Check that the fixed cells of `board` are legal.
 */
static bool_t check_fixed_cells(const board_t* board) {
    board_t fixed;
    bool_t ret;

    clone_fixed(&fixed, board);
    ret = board_is_legal(&fixed);
    board_destroy(&fixed);

    return ret;
}

/**
 * Reset the flags of any fixed cells on the board.
 */
static void unfix_cells(board_t* board) {
    int block_size = board_block_size(board);

    int row;
    int col;

    for (row = 0; row < block_size; row++) {
        for (col = 0; col < block_size; col++) {
            cell_t* cell = board_access(board, row, col);
            if (cell_is_fixed(cell)) {
                board_access(board, row, col)->flags = CF_NONE;
            }
        }
    }
}

/**
 * Check that the game's board is legal, printing an error if it isn't.
 * Returns whether the board is legal.
 */
static bool_t game_verify_board_legal(const game_t* game) {
    if (board_is_legal(&game->board)) {
        return TRUE;
    }

    print_error("Board is illegal.");

    if (!should_mark_errors(game)) {
        print_success("Note: use `mark_errors 1` to mark conflicting cells on "
                      "the board.");
    }

    return FALSE;
}

/**
 * Verify that the provided status is successful, printing an error if it isn't.
 */
static bool_t verify_lp_status(lp_status_t status) {
    switch (status) {
    case LP_SUCCESS:
        return TRUE;
    case LP_INFEASIBLE:
        print_error("Board is not solvable");
        return FALSE;
    case LP_GUROBI_ERR:
        print_error("Failed to invoke Gurobi");
        return FALSE;
    }
    return FALSE;
}

/**
 * Validate the current board using the ILP solver. If an unexpected error
 * occurs in the solver, print an error message and return false. Otherwise,
 * return true and set `valid` appropriately.
 *
 * Note: this function does not check the legality of the board. Use
 * `check_board_legal` to do so before calling this function.
 */
static bool_t game_validate_board(game_t* game, bool_t* valid) {
    switch (lp_validate_ilp(game->lp_env, &game->board)) {
    case LP_SUCCESS:
        *valid = TRUE;
        return TRUE;
    case LP_INFEASIBLE:
        *valid = FALSE;
        return TRUE;
    case LP_GUROBI_ERR:
        verify_lp_status(LP_GUROBI_ERR);
        return FALSE;
    }

    return FALSE;
}

/**
 * Check that `row` and `col` are legal (zero-based) indices for `board`,
 * printing an error message if they aren't.
 */
static bool_t verify_board_indices(const board_t* board, int row, int col) {
    int block_size = board_block_size(board);

    if (col < 0 || col >= block_size) {
        print_error("Illegal column value.");
        return FALSE;
    }

    if (row < 0 || row >= block_size) {
        print_error("Illegal row value.");
        return FALSE;
    }

    return TRUE;
}

/**
 * Delta application callback that prints the current change to the user.
 */
static void user_notify_delta_callback(int row, int col, int old, int new) {
    print_success("(%d, %d): %d -> %d", col + 1, row + 1, old, new);
}

/**
 * Apply `delta` to the game's board, printing it, and store the delta in
 * history. Ownership of the delta's contents is transferred to the game.
 *
 * If `print_changes` is true, the changes will be printed to the user as they
 * are applied.
 */
static void game_apply_delta(game_t* game, delta_list_t* delta,
                             bool_t print_changes) {
    delta_list_apply(&game->board, delta,
                     print_changes ? user_notify_delta_callback : NULL);
    history_add_item(&game->history, delta);

    game_board_after_change(game);
}

/**
 * Check that the game board is legal and solvable in preparation for saving
 * from edit mode, printing an appropriate error message if it isn't.
 */
static bool_t game_verify_save_edit_board(game_t* game) {
    bool_t valid;

    if (!game_verify_board_legal(game)) {
        return FALSE;
    }

    if (!game_validate_board(game, &valid)) {
        return FALSE;
    }

    if (!valid) {
        print_error("Board is not solvable.");
        return FALSE;
    }

    return TRUE;
}

/**
 * Mark any nonempty cells in `board` as fixed.
 */
static void fix_nonempty_cells(board_t* board) {
    int block_size = board_block_size(board);

    int row;
    int col;

    for (row = 0; row < block_size; row++) {
        for (col = 0; col < block_size; col++) {
            cell_t* cell = board_access(board, row, col);

            if (!cell_is_empty(cell)) {
                cell->flags = CF_FIXED;
            }
        }
    }
}

/**
 * Attempt to save `board` to `filename`, printing a message on error.
 */
static bool_t save_board_to_file(const board_t* board, const char* filename) {
    FILE* file = open_file(filename, "w");

    if (!file) {
        return FALSE;
    }

    board_serialize(board, file);
    fclose(file);
    return TRUE;
}

/**
 * Check the preconditions for hint commands - row and col point to a legal,
 * non-fixed, empty cell, printing an error if they aren't met.
 */
static bool_t game_verify_can_hint(const game_t* game, int row, int col) {
    const cell_t* cell;

    if (!game_verify_board_legal(game)) {
        return FALSE;
    }

    if (!verify_board_indices(&game->board, row, col)) {
        return FALSE;
    }

    cell = board_access_const(&game->board, row, col);

    if (cell_is_fixed(cell)) {
        print_error("Cannot provide hint for fixed cell");
        return FALSE;
    }

    if (!cell_is_empty(cell)) {
        print_error("Cannot provide hint for non-empty cell");
        return FALSE;
    }

    return TRUE;
}

/**
 * Add all empty cells that only have a single legal value to `delta` (setting
 * them to their legal value).
 */
static void add_autofill_candidates(delta_list_t* delta, board_t* board) {
    int block_size = board_block_size(board);

    int row;
    int col;

    for (row = 0; row < block_size; row++) {
        for (col = 0; col < block_size; col++) {
            if (cell_is_empty(board_access(board, row, col))) {
                int candidate;
                if (board_get_single_candidate(board, row, col, &candidate)) {
                    delta_list_add(delta, row, col, 0, candidate);
                }
            }
        }
    }
}

bool_t command_execute(game_t* game, command_t* command) {
    switch (command->type) {
    case CT_SOLVE: {
        board_t board;

        char* filename = command->arg.str_val;
        bool_t loaded = load_board_from_file(&board, filename);
        free(filename);

        if (!loaded) {
            break;
        }

        if (!check_fixed_cells(&board)) {
            print_error("The board's fixed cells are illegally placed.");
            board_destroy(&board);
            break;
        }

        enter_game_mode(game, GM_SOLVE, &board);
        game_board_after_change(game);

        break;
    }
    case CT_EDIT: {
        board_t board;
        char* filename = command->arg.str_val;

        if (filename) {
            bool_t loaded = load_board_from_file(&board, filename);
            free(filename);

            if (!loaded) {
                break;
            }

            /* Fixed cells have no meaning here. */
            unfix_cells(&board);
        } else {
            board_init(&board, 3, 3);
        }

        enter_game_mode(game, GM_EDIT, &board);
        game_board_after_change(game);

        break;
    }
    case CT_MARK_ERRORS:
        game->mark_errors = command->arg.bool_val;
        print_success("Errors will %sbe marked.",
                      game->mark_errors ? "" : "not ");
        break;

    case CT_PRINT_BOARD:
        game_board_print(game);
        break;

    case CT_SET: {
        int block_size = board_block_size(&game->board);

        int col = command->arg.three_int_val.i - 1;
        int row = command->arg.three_int_val.j - 1;
        int val = command->arg.three_int_val.k;

        cell_t* cell;
        delta_list_t updates;

        if (!verify_board_indices(&game->board, row, col)) {
            break;
        }

        if (val < 0 || val > block_size) {
            print_error("The value is out of range.");
            break;
        }

        cell = board_access(&game->board, row, col);

        if (cell_is_fixed(cell)) {
            print_error("This cell is fixed and cannot be updated.");
            break;
        }

        delta_list_init(&updates);
        delta_list_add(&updates, row, col, cell->value, val);
        game_apply_delta(game, &updates, FALSE);

        break;
    }
    case CT_VALIDATE: {
        bool_t valid;

        if (!game_verify_board_legal(game)) {
            break;
        }

        if (!game_validate_board(game, &valid)) {
            break;
        }

        if (valid) {
            print_success("Board is solvable.");
        } else {
            print_success("Board is not solvable.");
        }

        break;
    }
    case CT_GUESS: {
        board_t guess;
        lp_status_t status;
        delta_list_t list;
        float thresh = command->arg.float_val;

        if (!game_verify_board_legal(game)) {
            break;
        }

        board_clone(&guess, &game->board);
        status = lp_guess_continuous(game->lp_env, &guess, thresh);

        if (verify_lp_status(status)) {
            delta_list_set_diff(&list, &game->board, &guess);
            game_apply_delta(game, &list, FALSE);
        }

        board_destroy(&guess);
        break;
    }

    case CT_GENERATE: {
        int x = command->arg.two_int_val.i;
        int y = command->arg.two_int_val.j;
        int block_size = board_block_size(&game->board);
        board_t generated;
        lp_gen_status_t status;

        if (x < 0 || x > block_size * block_size) {
            print_error("Amount of empty cells is out of range.");
            break;
        } else if (y <= 0 || y > block_size * block_size) {
            print_error("Amount of remaining cells is out of range.");
            break;
        }

        board_clone(&generated, &game->board);

        status = lp_gen_ilp(game->lp_env, &generated, x, y);

        switch (status) {
        case GEN_SUCCESS: {
            delta_list_t list;
            delta_list_set_diff(&list, &game->board, &generated);
            game_apply_delta(game, &list, FALSE);
            break;
        }
        case GEN_MAX_ATTEMPTS:
            print_error("Reached the maximum amount of attempts (1000).");
            break;

        case GEN_GUROBI_ERR:
            verify_lp_status(LP_GUROBI_ERR);
            break;

        case GEN_TOO_FEW_EMPTY:
            print_error("Board does not contain %d empty cells.", x);
            break;
        }

        board_destroy(&generated);
        break;
    }
    case CT_UNDO: {
        const delta_list_t* delta = history_undo(&game->history);
        if (!delta) {
            print_error("Nothing to undo.");
            break;
        }

        delta_list_revert(&game->board, delta, user_notify_delta_callback);
        game_board_after_change(game);

        break;
    }
    case CT_REDO: {
        const delta_list_t* delta = history_redo(&game->history);
        if (!delta) {
            print_error("Nothing to redo.");
            break;
        }

        delta_list_apply(&game->board, delta, user_notify_delta_callback);
        game_board_after_change(game);

        break;
    }
    case CT_SAVE: {
        char* filename = command->arg.str_val;
        bool_t succeeded;

        /* In edit mode, the board must be legal and solvable, and will become
         * fixed on save. */
        if (game->mode == GM_EDIT) {
            if (!game_verify_save_edit_board(game)) {
                free(filename);
                break;
            }

            fix_nonempty_cells(&game->board);
            succeeded = save_board_to_file(&game->board, filename);
            unfix_cells(&game->board);
        } else {
            succeeded = save_board_to_file(&game->board, filename);
        }

        if (succeeded) {
            print_success("Saved board to '%s'.", filename);
        }

        free(filename);

        break;
    }
    case CT_HINT: {
        int col = command->arg.two_int_val.i - 1;
        int row = command->arg.two_int_val.j - 1;

        lp_status_t status;
        board_t solution;

        if (!game_verify_can_hint(game, row, col)) {
            break;
        }

        board_clone(&solution, &game->board);

        status = lp_solve_ilp(game->lp_env, &solution);

        if (verify_lp_status(status)) {
            print_success("Set (%d, %d) to %d", col + 1, row + 1,
                          board_access(&solution, row, col)->value);
        }

        board_destroy(&solution);
        break;
    }
    case CT_GUESS_HINT: {
        int col = command->arg.two_int_val.i - 1;
        int row = command->arg.two_int_val.j - 1;

        int block_size = board_block_size(&game->board);
        int i;

        lp_cell_candidates_t *candidate_board, *candidates;
        lp_status_t status;

        if (!game_verify_can_hint(game, row, col)) {
            break;
        }
        candidate_board = checked_calloc(block_size * block_size,
                                         sizeof(lp_cell_candidates_t));

        status =
            lp_solve_continuous(game->lp_env, &game->board, candidate_board);

        if (!verify_lp_status(status)) {
            goto cleanup_gh;
        }

        candidates = &candidate_board[row * block_size + col];

        if (candidates->size == 0) {
            print_success("No candidates found.");
        } else {
            print_success("Available candidates (score):");
        }

        for (i = 0; i < candidates->size; i++) {
            lp_candidate_t* candidate = &candidates->candidates[i];
            print_success("%d (%f)", candidate->val, candidate->score);
        }

    cleanup_gh:
        lp_cell_candidates_array_destroy(candidate_board, block_size);
        break;
    }

    case CT_NUM_SOLUTIONS:
        print_success("Number of solutions: %d", num_solutions(&game->board));
        break;

    case CT_AUTOFILL: {
        delta_list_t delta;
        delta_list_init(&delta);
        add_autofill_candidates(&delta, &game->board);
        game_apply_delta(game, &delta, TRUE);
        break;
    }

    case CT_RESET: {
        const delta_list_t* delta;

        while ((delta = history_undo(&game->history))) {
            delta_list_revert(&game->board, delta, NULL);
        }

        game_board_after_change(game);
        break;
    }
    case CT_EXIT:
        print_success("Exiting...");
        return FALSE;
    }

    return TRUE;
}
