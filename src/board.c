#include "board.h"

#include "bool.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

bool_t cell_is_empty(const cell_t* cell) { return cell->value == 0; }
bool_t cell_is_fixed(const cell_t* cell) {
    return cell->flags == CELL_FLAGS_FIXED;
}
bool_t cell_is_error(const cell_t* cell) {
    return cell->flags == CELL_FLAGS_ERROR;
}

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

const cell_t* board_access_const(const board_t* board, int row, int col) {
    return board_access((board_t*)board, row, col);
}

cell_t* board_access_block(board_t* board, int block_row, int block_col,
                           int local_row, int local_col) {
    return board_access(board, block_row * board->m + local_row,
                        block_col * board->n + local_col);
}

const cell_t* board_access_block_const(const board_t* board, int block_row,
                                       int block_col, int local_row,
                                       int local_col) {
    return board_access_block((board_t*)board, block_row, block_col, local_row,
                              local_col);
}

static void print_separator_line(int m, int n, FILE* stream) {
    int line_len = 4 * n * m + m + 1;
    int i;
    for (i = 0; i < line_len; i++) {
        fputc('-', stream);
    }
    fputc('\n', stream);
}

static void print_cell(const cell_t* cell, FILE* stream) {
    fputc(' ', stream);
    if (cell_is_empty(cell)) {
        fprintf(stream, "   ");
    } else {
        char decorator =
            cell_is_fixed(cell) ? '.' : cell_is_error(cell) ? '*' : ' ';
        fprintf(stream, "%2d%c", cell->value, decorator);
    }
}

void board_print(const board_t* board, FILE* stream) {
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
            print_cell(board_access_const(board, row, col), stream);
        }

        fputs("|\n", stream);
    }

    print_separator_line(board->m, board->n, stream);
}

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
            cell->flags = CELL_FLAGS_FIXED;
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

    if (!board_init(board, m, n)) {
        return DS_ERR_IO; /* TODO: just terminate on allocation failures */
    }

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
