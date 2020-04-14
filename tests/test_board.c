#include "board.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_board_access() {
    board_t board;

    assert(board_init(&board, 2, 5));
    assert(board_block_size(&board) == 10);

    assert(board.cells[13].value == 0);
    board_access(&board, 1, 3)->value = 17;
    assert(board.cells[13].value == 17);
}

static void test_board_print() {
    board_t board;
    cell_t* cell;

    FILE* stream;
    char buf[1024] = {0};
    const char* expected = "-------------------------------------------\n"
                           "|              5.    |                    |\n"
                           "|                  5*|                    |\n"
                           "-------------------------------------------\n"
                           "|                    |      6             |\n"
                           "|                    |  7   8             |\n"
                           "-------------------------------------------\n"
                           "|                    |                    |\n"
                           "|                    |                    |\n"
                           "-------------------------------------------\n"
                           "|                    |                    |\n"
                           "|          3         |                    |\n"
                           "-------------------------------------------\n"
                           "|                    |                    |\n"
                           "|                    |                    |\n"
                           "-------------------------------------------\n";

    assert(board_init(&board, 2, 5));
    cell = board_access(&board, 0, 3);
    cell->value = 5;
    cell->flags = CELL_FLAGS_FIXED;
    cell = board_access(&board, 1, 4);
    cell->value = 5;
    cell->flags = CELL_FLAGS_ERROR;
    cell = board_access(&board, 2, 6);
    cell->value = 6;
    cell = board_access(&board, 3, 5);
    cell->value = 7;
    cell = board_access(&board, 3, 6);
    cell->value = 8;
    cell = board_access(&board, 7, 2);
    cell->value = 3;

    stream = tmpfile();
    assert(stream);

    board_print(&board, stream);
    rewind(stream);
    fread(buf, 1, sizeof(buf), stream);
    fputs(buf, stderr);

    assert(!strcmp(buf, expected));
}

static void test_board_serialize() {
    board_t board;
    int row;
    int col;

    FILE* stream;
    char buf[256] = {0};
    const char* expected = "3 2\n"
                           "1 2 3 4 5 6\n"
                           "2 3 4 5 6 1\n"
                           "3 4 5 6 1. 2\n"
                           "4 5 6 1. 2 3\n"
                           "5 6 1 2 3 4\n"
                           "6 1 2 3 4 5\n";

    assert(board_init(&board, 3, 2));

    for (row = 0; row < 6; row++) {
        for (col = 0; col < 6; col++) {
            board_access(&board, row, col)->value = (row + col) % 6 + 1;
        }
    }

    board_access(&board, 2, 4)->flags = CELL_FLAGS_FIXED;
    board_access(&board, 3, 3)->flags = CELL_FLAGS_FIXED;

    stream = tmpfile();

    board_serialize(&board, stream);
    rewind(stream);
    fread(buf, 1, sizeof(buf), stream);
    fputs(buf, stderr);

    assert(!strcmp(buf, expected));
}

static void test_board_deserialize() {
    board_t board;
    int row;
    int col;

    FILE* stream;
    const char* contents = "3 2\n"
                           "1 2 3 4 5 6\n"
                           "2 3 4   5 6 1\n"
                           "3 4 5 6 1. 2\n"
                           "4 5\t6 1. 2 3\n"
                           "5 6 1 2 3 4"
                           "  6 1 2 3 4 5\n";

    stream = tmpfile();
    fputs(contents, stream);
    rewind(stream);

    assert(board_deserialize(&board, stream));

    for (row = 0; row < 6; row++) {
        for (col = 0; col < 6; col++) {
            const cell_t* cell = board_access_const(&board, row, col);

            assert(cell->value == (row + col) % 6 + 1);

            if ((row == 2 && col == 4) || (row == 3 && col == 3)) {
                assert(cell->flags == CELL_FLAGS_FIXED);
            } else {
                assert(cell->flags == CELL_FLAGS_NONE);
            }
        }
    }
}

int main() {
    test_board_access();
    test_board_print();
    test_board_serialize();
    test_board_deserialize();
    return 0;
}
