#include "board.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

bool_t board_init(board_t* board, int m, int n) {
    int block_size = m * n;
    cell_t* cells = calloc(block_size * block_size, sizeof(cell_t));

    if (!cells) {
        return FALSE;
    }

    board->cells = cells;
    board->m = m;
    board->n = n;

    return TRUE;
}

void board_destroy(board_t* board) { free(board->cells); }

int board_block_size(const board_t* board) { return board->m * board->n; }

cell_t* board_access(board_t* board, int row, int col) {
    return &board->cells[row * board_block_size(board) + col];
}
const cell_t* board_access_const(const board_t* board, int row, int col){
    return board_access((board_t*)board, row, col);
}

cell_t* board_access_block(board_t* board, int row, int col, int blockrow,
                           int blockcol) {
    int block_start =
        blockrow * board_block_size(board) * board->m + blockcol * board->n;
    return &board->cells[block_start + row * board_block_size(board) + col];
}
const cell_t* board_access_block_const(const board_t* board, int row, int col,
                                       int blockrow, int blockcol) {
    return board_access_block((board_t*)board, row, col, blockrow, blockcol);
}

static void print_separator_line(int n, int m, FILE* stream) {
    int line_len = 4 * n * m + m + 1;
    int i;
    for (i = 0; i < line_len; i++) {
        fputc('-', stream);
    }
    fputc('\n', stream);
}

void board_print(const board_t* board, FILE* stream) {
    int bi, bj, i, j;
    for (bi = 0; bi < board->n; bi++) {
        print_separator_line(board->n, board->m, stream);
        for (i = 0; i < board->m; i++) {
            fprintf(stream, "|");
            for (bj = 0; bj < board->m; bj++) {
                for (j = 0; j < board->n; j++) {
                    const cell_t* cell =
                        board_access_block_const(board, i, j, bi, bj);
                    fputc(' ', stream);
                    if (cell->value > 0) {
                        fprintf(stream, "%2d%c", cell->value,
                                cell->flags == CELL_FLAGS_FIXED
                                    ? '.'
                                    : (cell->flags == CELL_FLAGS_ERROR ? '*'
                                                                       : ' '));
                    } else {
                        fprintf(stream, "   ");
                    }
                }
                fprintf(stream, "|");
            }
        }
    }
    print_separator_line(board->n, board->m, stream);
}

void board_serialize(const board_t* board, FILE* stream) {
    int block_size = board_block_size(board);
    int row;
    int col;

    fprintf(stream, "%d %d\n", board->m, board->n);

    for (row = 0; row < block_size; row++) {
        for (col = 0; col < block_size; col++) {
            const cell_t* cell = board_access_const(board, row, col);
            if (col > 0) {
                fputc(' ', stream);
            }
            fprintf(stream, "%d", cell->value);
        }
        fputs("", stream);
    }
}
