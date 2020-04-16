/**
 * board.h - Board structures and helpers, including error marking, printing and
 * [de]serialization.
 */

#ifndef BOARD_H
#define BOARD_H

#include "bool.h"
#include <stddef.h>
#include <stdio.h>

/**
 * Additional flags that can be attached to a cell.
 */
typedef enum cell_flags {
    CF_NONE,  /* No additional information */
    CF_FIXED, /* Cell is fixed */
    CF_ERROR  /* Cell has been identified as an error */
} cell_flags_t;

/**
 * Represents a single cell on the board.
 */
typedef struct cell {
    int value;
    cell_flags_t flags;
} cell_t;

/**
 * Represents a board containing n rows of m blocks, where each block contains m
 * rows of n cells ((nm)^2 cells in total).
 */
typedef struct board {
    cell_t* cells;
    int m;
    int n;
} board_t;

/**
 * Check whether `cell` is empty (has value 0).
 */
bool_t cell_is_empty(const cell_t* cell);

/**
 * Check whether `cell` is fixed.
 */
bool_t cell_is_fixed(const cell_t* cell);

/**
 * Check whether `cell` has been marked as an error.
 */
bool_t cell_is_error(const cell_t* cell);

/**
 * Initialize a new board with the specified `m` and `n`.
 */
void board_init(board_t* board, int m, int n);

/**
 * Destroy `board`, releasing any allocated resources.
 */
void board_destroy(board_t* board);

/**
 * Clone `src` into `dest`.
 */
void board_clone(board_t* dest, const board_t* src);

/**
 * Retrieve the block size of `board`. This is also the number of rows and
 * columns on the board.
 */
int board_block_size(const board_t* board);

/**
 * Retrieve the cell at the specified row and column on the board.
 */
cell_t* board_access(board_t* board, int row, int col);
const cell_t* board_access_const(const board_t* board, int row, int col);

/**
 * Retrieve the cell at the specified position within the specified block on the
 * board.
 */
cell_t* board_access_block(board_t* board, int block_row, int block_col,
                           int local_row, int local_col);

const cell_t* board_access_block_const(const board_t* board, int block_row,
                                       int block_col, int local_row,
                                       int local_col);

/**
 * Check whether `board` is legal (in the sense that no two "neighbors" share
 * the same value), marking any conflicting non-fixed cells as errors.
 */
bool_t board_check_legal(board_t* board);

/**
 * Print `board` to `stream` in a human-readable format. If `mark_errors` is
 * true, erroneous cells (those with `CF_ERROR` set) will be printed with an
 * asterisk.
 */
void board_print(const board_t* board, FILE* stream, bool_t mark_errors);

/**
 * Status code returned from deserialization operations.
 */
typedef enum deserialize_status {
    DS_OK,      /* Deserialization succeeded */
    DS_ERR_IO,  /* Unknown IO error */
    DS_ERR_FMT, /* Bad file format */
    DS_ERR_CELL /* Invalid cell encountered */
} deserialize_status_t;

/**
 * Serialize `board` to the specified stream.
 */
void board_serialize(const board_t* board, FILE* stream);

/**
 * Deserialize into `board` from the specified stream. If the call succeeds
 * (status `DS_OK`), the board should be cleaned up with `board_destroy` after
 * use.
 * Note that this function does not check the legality of the resulting board in
 * any way.
 */
deserialize_status_t board_deserialize(board_t* board, FILE* stream);

#endif
