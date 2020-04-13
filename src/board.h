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

typedef struct cell {
    int value;
    cell_flags_t flags;
} cell_t;

typedef struct board {
    cell_t* cells;
    int m;
    int n;
} board_t;

bool_t board_init(board_t* board, int m, int n);
void board_destroy(board_t* board);

int board_block_size(const board_t* board);

cell_t* board_access(board_t* board, int row, int col);
const cell_t* board_access_const(const board_t* board, int row, int col);

cell_t* board_access_block(board_t* board, int row, int col, int blockrow,
                           int blockcol);
const cell_t* board_access_block_const(const board_t* board, int row, int col,
                                       int blockrow, int blockcol);

void board_print(const board_t* board, FILE* stream);

void board_serialize(const board_t* board, FILE* stream);
bool_t board_deserialize(board_t* board, FILE* stream);

#endif
