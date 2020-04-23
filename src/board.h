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
 * Compute the row in which the cell at the specified position within the
 * specified block resides.
 */
int board_block_row(const board_t* board, int block_row, int local_row);

/**
 * Compute the column in which the cell at the specified position within the
 * specified block resides.
 */
int board_block_col(const board_t* board, int block_col, int local_col);

/**
 * Retrieve the cell at the specified row and column on the board.
 */
cell_t* board_access(board_t* board, int row, int col);
const cell_t* board_access_const(const board_t* board, int row, int col);

/**
 * Check whether `board` is legal (in the sense that no two "neighbors" share
 * the same value).
 */
bool_t board_is_legal(const board_t* board);

/**
 * Mark conflicting non-fixed cells on the board as errors.
 */
void board_mark_errors(board_t* board);

/**
 * Gather all possible legal values for the specified position on the board,
 * storing them to `candidates`, and return the number of candidates found.
 * `candidates` should have `block_size` entries, to be safe.
 *
 * Note: the specified cell's contents will be overriden, and it will be emptied
 * when the function returns.
 */
int board_gather_candidates(board_t* board, int row, int col, int* candidates);

/**
 * Check whether the specified position on the board has only a single legal
 * value, storing it to `candidate` and returning true if so.
 *
 * Note: the specified cell's contents will be overriden, and it will be emptied
 * when the function returns.
 */
bool_t board_get_single_candidate(board_t* board, int row, int col,
                                  int* candidate);

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
