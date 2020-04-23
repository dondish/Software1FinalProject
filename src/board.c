#include "board.h"

#include "bool.h"
#include "checked_alloc.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool_t cell_is_empty(const cell_t* cell) { return cell->value == 0; }
bool_t cell_is_fixed(const cell_t* cell) { return cell->flags == CF_FIXED; }
bool_t cell_is_error(const cell_t* cell) { return cell->flags == CF_ERROR; }

void board_init(board_t* board, int m, int n) {
    int block_size = m * n;
    cell_t* cells = checked_calloc(block_size * block_size, sizeof(cell_t));

    board->cells = cells;
    board->m = m;
    board->n = n;
}

void board_destroy(board_t* board) { free(board->cells); }

void board_clone(board_t* dest, const board_t* src) {
    int block_size = board_block_size(src);

    board_init(dest, src->m, src->n);
    memcpy(dest->cells, src->cells, block_size * block_size * sizeof(cell_t));
}

int board_block_size(const board_t* board) { return board->m * board->n; }

int board_block_row(const board_t* board, int block_row, int local_row) {
    return block_row * board->m + local_row;
}

int board_block_col(const board_t* board, int block_col, int local_col) {
    return block_col * board->n + local_col;
}

cell_t* board_access(board_t* board, int row, int col) {
    return &board->cells[row * board_block_size(board) + col];
}

const cell_t* board_access_const(const board_t* board, int row, int col) {
    return board_access((board_t*)board, row, col);
}

/* LEGALITY CHECKS/ERROR MARKING */

/**
 * Abstraction for a function that can retrieve a cell based on two indices (a
 * "context" and a "local offset", both in the range `[0, block_size)`), used in
 * the generalized legality algorithm to abstract row, column and block checks.
 */
typedef cell_t* (*cell_retriever_t)(board_t* board, int ctx, int local_off);

/**
 * Callback type invoked when two conflicting cells are found. If the callback
 * returns false, processing is halted and the legality check returns false as
 * well.
 */
typedef bool_t (*cell_conflict_handler_t)(cell_t* a, cell_t* b);

/**
 * Value map item, used to track the last cell on the board in which a given
 * value was encountered. The value map itself is an array of these, indexed by
 * cell value.
 */
typedef struct {
    bool_t occupied;
    int local_off;
} val_map_item_t;

/**
 * Check that each of the non-empty cells retrieved by `retriever` has a
 * distinct value, invoking `handler` on conflicting cells.
 *
 * If `handler` returns false for a given pair of cells, no further checks are
 * performed and the function returns false. Otherwise, true is returned.
 *
 * `ctx` is held constant while `local_off` is iterated across
 * `[0, block_size)`.
 *
 * `map` is used as scratch storage during the check, and must contain at least
 * `block_size` items.
 */
static bool_t check_legal(board_t* board, val_map_item_t* map, int ctx,
                          cell_retriever_t retrieve,
                          cell_conflict_handler_t handler) {
    int block_size = board_block_size(board);
    int local_off;

    memset(map, 0, block_size * sizeof(val_map_item_t));

    for (local_off = 0; local_off < block_size; local_off++) {
        cell_t* cell = retrieve(board, ctx, local_off);

        if (!cell_is_empty(cell)) {
            val_map_item_t* item = map + (cell->value - 1);

            if (item->occupied) {
                /* We've seen this value before - report the conflict. */
                cell_t* old_cell = retrieve(board, ctx, item->local_off);
                if (!handler(old_cell, cell)) {
                    return FALSE;
                }
            }

            item->occupied = TRUE;
            item->local_off = local_off;
        }
    }

    return TRUE;
}

/**
 * Retriever for checking legality in a row - use `local_off` to index into the
 * row designated by `ctx`.
 */
static cell_t* retrieve_by_row(board_t* board, int ctx, int local_off) {
    return board_access(board, ctx, local_off);
}

/**
 * Retriever for checking legality in a column - use `local_off` to index into
 * the column designated by `ctx`.
 */
static cell_t* retrieve_by_col(board_t* board, int ctx, int local_off) {
    return board_access(board, local_off, ctx);
}

/**
 * Retriever for checking legality in a block - use `local_off` to index into
 * the block designated by `ctx` (in row-major order).
 */
static cell_t* retrieve_by_block(board_t* board, int ctx, int local_off) {
    int block_row = ctx / board->m;
    int block_col = ctx % board->m;

    int local_row = local_off / board->n;
    int local_col = local_off % board->n;

    return board_access(board, board_block_row(board, block_row, local_row),
                        board_block_col(board, block_col, local_col));
}

/**
 * Conflict handler that aborts further checking of the board.
 */
static bool_t abort_check_handler(cell_t* a, cell_t* b) {
    (void)a;
    (void)b;
    return FALSE;
}

bool_t board_is_legal(const board_t* board) {
    board_t* nonconst_board = (board_t*)board;

    int block_size = board_block_size(board);
    val_map_item_t* map = checked_calloc(block_size, sizeof(val_map_item_t));

    int i;
    bool_t ret = TRUE;

    for (i = 0; i < block_size; i++) {
        ret = check_legal(nonconst_board, map, i, retrieve_by_row,
                          abort_check_handler);
        if (!ret) {
            break;
        }

        ret = check_legal(nonconst_board, map, i, retrieve_by_col,
                          abort_check_handler);
        if (!ret) {
            break;
        }

        ret = check_legal(nonconst_board, map, i, retrieve_by_block,
                          abort_check_handler);
        if (!ret) {
            break;
        }
    }

    free(map);
    return ret;
}

/* ERROR MARKING */

static void clear_errors(board_t* board) {
    int block_size = board_block_size(board);

    int row;
    int col;

    for (row = 0; row < block_size; row++) {
        for (col = 0; col < block_size; col++) {
            cell_t* cell = board_access(board, row, col);
            if (cell_is_error(cell)) {
                cell->flags = CF_NONE;
            }
        }
    }
}

/**
 * Mark `cell` as an error if it is not fixed.
 */
static void cell_mark_error(cell_t* cell) {
    if (!cell_is_fixed(cell)) {
        cell->flags = CF_ERROR;
    }
}

/**
 * Conflict handler that marks offending cells as errors and continues
 * processing.
 */
static bool_t mark_errors_handler(cell_t* a, cell_t* b) {
    cell_mark_error(a);
    cell_mark_error(b);
    return TRUE;
}

void board_mark_errors(board_t* board) {
    int block_size = board_block_size(board);
    val_map_item_t* map = checked_calloc(block_size, sizeof(val_map_item_t));

    int i;

    clear_errors(board);

    for (i = 0; i < block_size; i++) {
        check_legal(board, map, i, retrieve_by_row, mark_errors_handler);
        check_legal(board, map, i, retrieve_by_col, mark_errors_handler);
        check_legal(board, map, i, retrieve_by_block, mark_errors_handler);
    }

    free(map);
}

/* PRINTING */

static void print_separator_line(int m, int n, FILE* stream) {
    int line_len = 4 * n * m + m + 1;
    int i;
    for (i = 0; i < line_len; i++) {
        fputc('-', stream);
    }
    fputc('\n', stream);
}

static void print_cell(const cell_t* cell, FILE* stream, bool_t mark_errors) {
    fputc(' ', stream);
    if (cell_is_empty(cell)) {
        fprintf(stream, "   ");
    } else {
        char decorator = cell_is_fixed(cell)
                             ? '.'
                             : mark_errors && cell_is_error(cell) ? '*' : ' ';
        fprintf(stream, "%2d%c", cell->value, decorator);
    }
}

void board_print(const board_t* board, FILE* stream, bool_t mark_errors) {
    int block_size = board_block_size(board);
    int row;
    int col;

    for (row = 0; row < block_size; row++) {
        if (row % board->m == 0) {
            print_separator_line(board->m, board->n, stream);
        }

        for (col = 0; col < block_size; col++) {
            if (col % board->n == 0) {
                fputc('|', stream);
            }
            print_cell(board_access_const(board, row, col), stream,
                       mark_errors);
        }

        fputs("|\n", stream);
    }

    print_separator_line(board->m, board->n, stream);
}

/* SERIALIZATION/DESERIALIZATION */

static void serialize_cell(const cell_t* cell, FILE* stream) {
    fprintf(stream, "%d", cell->value);
    if (cell_is_fixed(cell)) {
        fputc('.', stream);
    }
}

void board_serialize(const board_t* board, FILE* stream) {
    int block_size = board_block_size(board);
    int row;
    int col;

    fprintf(stream, "%d %d\n", board->m, board->n);

    for (row = 0; row < block_size; row++) {
        for (col = 0; col < block_size; col++) {
            if (col > 0) {
                fputc(' ', stream);
            }
            serialize_cell(board_access_const(board, row, col), stream);
        }
        fputc('\n', stream);
    }
}

static deserialize_status_t handle_scanf_err(FILE* stream) {
    return ferror(stream) ? DS_ERR_IO : DS_ERR_FMT;
}

static deserialize_status_t deserialize_cell(cell_t* cell, int block_size,
                                             FILE* stream) {
    int value;
    int next_char;

    if (fscanf(stream, " %d", &value) < 1) {
        return handle_scanf_err(stream);
    }

    if (value < 0 || value > block_size) {
        return DS_ERR_CELL;
    }

    cell->value = value;

    next_char = fgetc(stream);

    if (next_char == '.') {
        if (!value) {
            /* fixed empty cell */
            return DS_ERR_CELL;
        } else {
            cell->flags = CF_FIXED;
        }
    } else {
        ungetc(next_char, stream);
    }

    return DS_OK;
}

deserialize_status_t board_deserialize(board_t* board, FILE* stream) {
    int m, n;
    int block_size;

    int row;
    int col;

    if (fscanf(stream, "%d %d", &m, &n) < 2) {
        return handle_scanf_err(stream);
    }

    if (m <= 0 || n <= 0) {
        return DS_ERR_FMT;
    }

    block_size = m * n;
    board_init(board, m, n);

    for (row = 0; row < block_size; row++) {
        for (col = 0; col < block_size; col++) {
            cell_t* cell = board_access(board, row, col);
            deserialize_status_t cell_status =
                deserialize_cell(cell, block_size, stream);

            if (cell_status != DS_OK) {
                board_destroy(board);
                return cell_status;
            }
        }
    }

    return DS_OK;
}
