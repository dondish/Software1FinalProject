#ifndef BOARD_H
#define BOARD_H

#include "bool.h"
#include <stddef.h>
#include <stdio.h>

typedef enum cell_flags {
    CELL_FLAGS_NONE,
    CELL_FLAGS_FIXED,
    CELL_FLAGS_ERROR
} cell_flags_t;

typedef enum deserialize_status {
    DS_OK,
    DS_ERR_IO,
    DS_ERR_FMT,
    DS_ERR_CELL
} deserialize_status_t;

typedef struct cell {
    int value;
    cell_flags_t flags;
} cell_t;

typedef struct board {
    cell_t* cells;
    int m;
    int n;
} board_t;

bool_t cell_is_empty(const cell_t* cell);
bool_t cell_is_fixed(const cell_t* cell);
bool_t cell_is_error(const cell_t* cell);

void board_init(board_t* board, int m, int n);
void board_destroy(board_t* board);

int board_block_size(const board_t* board);

cell_t* board_access(board_t* board, int row, int col);
const cell_t* board_access_const(const board_t* board, int row, int col);

cell_t* board_access_block(board_t* board, int block_row, int block_col,
                           int local_row, int local_col);

const cell_t* board_access_block_const(const board_t* board, int block_row,
                                       int block_col, int local_row,
                                       int local_col);

void board_print(const board_t* board, FILE* stream);

void board_serialize(const board_t* board, FILE* stream);
deserialize_status_t board_deserialize(board_t* board, FILE* stream);

#endif
