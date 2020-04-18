#include "board.h"

#include "bool.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static void test_board_access(void) {
    board_t board;

    board_init(&board, 2, 5);
    assert(board_block_size(&board) == 10);

    assert(board.cells[13].value == 0);
    board_access(&board, 1, 3)->value = 17;
    assert(board.cells[13].value == 17);

    board.cells[27].value = 23;
    assert(board_access_block(&board, 1, 1, 0, 2)->value == 23);

    board.cells[73].value = 33;
    assert(board_access_block(&board, 3, 0, 1, 3)->value == 33);
}

static void check_contents(FILE* stream, const char* expected) {
    char buf[1024] = {0};

    rewind(stream);
    fread(buf, 1, sizeof(buf), stream);
    fclose(stream);
    fputs(buf, stderr);
    assert(!strcmp(buf, expected));
}

static void test_board_print(void) {
    board_t board;
    cell_t* cell;

    FILE* stream;
    const char* expected_marked =
        "-------------------------------------------\n"
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
    const char* expected_unmarked =
        "-------------------------------------------\n"
        "|              5.    |                    |\n"
        "|                  5 |                    |\n"
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

    board_init(&board, 2, 5);
    cell = board_access(&board, 0, 3);
    cell->value = 5;
    cell->flags = CF_FIXED;
    cell = board_access(&board, 1, 4);
    cell->value = 5;
    cell->flags = CF_ERROR;
    cell = board_access(&board, 2, 6);
    cell->value = 6;
    cell = board_access(&board, 3, 5);
    cell->value = 7;
    cell = board_access(&board, 3, 6);
    cell->value = 8;
    cell = board_access(&board, 7, 2);
    cell->value = 3;

    stream = tmpfile();
    board_print(&board, stream, TRUE);
    check_contents(stream, expected_marked);

    stream = tmpfile();
    board_print(&board, stream, FALSE);
    check_contents(stream, expected_unmarked);
}

static void test_board_serialize(void) {
    board_t board;
    int row;
    int col;

    FILE* stream;
    const char* expected = "3 2\n"
                           "1 2 3 4 5 6\n"
                           "2 3 4 5 6 1\n"
                           "3 4 5 6 1. 2\n"
                           "4 5 6 1. 2 3\n"
                           "5 6 1 2 3 4\n"
                           "6 1 2 3 4 5\n";

    board_init(&board, 3, 2);

    for (row = 0; row < 6; row++) {
        for (col = 0; col < 6; col++) {
            board_access(&board, row, col)->value = (row + col) % 6 + 1;
        }
    }

    board_access(&board, 2, 4)->flags = CF_FIXED;
    board_access(&board, 3, 3)->flags = CF_FIXED;

    stream = tmpfile();

    board_serialize(&board, stream);
    check_contents(stream, expected);
}

static FILE* fill_stream(const char* contents) {
    FILE* stream = tmpfile();
    fputs(contents, stream);
    rewind(stream);
    return stream;
}

static void test_board_deserialize(void) {
    board_t board;
    int row;
    int col;

    const char* contents = "3 2\n"
                           "1 2 3 4 5 6\n"
                           "2 3 4   5 6 1\n"
                           "3 4 5 6 1. 2\n"
                           "4 5\t6 1. 2 3\n"
                           "5 6 1 2 3 4"
                           "  6 1 2 3 4 5\n";

    FILE* stream = fill_stream(contents);

    int status = board_deserialize(&board, stream);
    fclose(stream);

    assert(status == DS_OK);

    assert(board.m == 3);
    assert(board.n == 2);

    for (row = 0; row < 6; row++) {
        for (col = 0; col < 6; col++) {
            const cell_t* cell = board_access_const(&board, row, col);

            assert(cell->value == (row + col) % 6 + 1);

            if ((row == 2 && col == 4) || (row == 3 && col == 3)) {
                assert(cell_is_fixed(cell));
            } else {
                assert(cell->flags == CF_NONE);
            }
        }
    }
}

static void test_board_deserialize_err_fmt(void) {
    const char* bad_contents[] = {
        "abcd", /* junk */

        "3 2\n" /* space before dot */
        "1 2 3 4 5 6\n"
        "2 3 4   5 6 1\n"
        "3 4 5 6 1. 2\n"
        "4 5\t6 1 . 2 3\n"
        "5 6 1 2 3 4"
        "  6 1 2 3 4 5\n",

        "3 2\n" /* missing values */
        "1 2 3 4 5 6\n"
        "2 3 4   5 6 1\n"
        "3 4 5 6 1. 2\n"
        "4 5\t6 1. 2 3\n"
        "5 6 1 2 3 4"
        "  6 1 2 \n",

        "-3 2\n" /* negative board sizes */
        "1 2 3 4 5 6\n"
        "2 3 4   5 6 1\n"
        "3 4 5 6 1. 2\n"
        "4 5\t6 1 . 2 3\n"
        "5 6 1 2 3 4"
        "  6 1 2 3 4 5\n",
    };

    size_t i;
    for (i = 0; i < sizeof(bad_contents) / sizeof(bad_contents[0]); i++) {
        board_t board;
        FILE* stream = fill_stream(bad_contents[i]);
        int status = board_deserialize(&board, stream);
        fclose(stream);
        assert(status == DS_ERR_FMT);
    }
}

static void test_board_deserialize_err_cell_val(void) {
    const char* bad_contents[] = {
        "3 2\n" /* negative cell value */
        "1 2 3 4 5 6\n"
        "2 3 4   5 6 1\n"
        "3 4 5 -6 1. 2\n"
        "4 5\t6 1. 2 3\n"
        "5 6 1 2 3 4"
        "  6 1 2 3 4 5\n",

        "3 2\n" /* negative cell value */
        "1 2 3 4 5 6\n"
        "2 3 4   5 6 1\n"
        "3 4 5 -6 1. 2\n"
        "4 5\t6 1. 2 3\n"
        "5 6 1 2 3 4"
        "  6 1 2 3 4 5\n",

        "3 2\n" /* cell value too large */
        "1 2 3 4 5 6\n"
        "2 3 4   5 6 1\n"
        "3 4 5 10 1. 2\n"
        "4 5\t6 1. 2 3\n"
        "5 6 1 2 3 4"
        "  6 1 2 3 4 5\n",

        "3 2\n" /* fixed empty cell */
        "1 2 3 4 5 6\n"
        "2 3 4   5 6 1\n"
        "3 4 5 6 1. 2\n"
        "4 5\t6 0. 2 3\n"
        "5 6 1 2 3 4"
        "  6 1 2 3 4 5\n",
    };

    size_t i;
    for (i = 0; i < sizeof(bad_contents) / sizeof(bad_contents[0]); i++) {
        board_t board;
        FILE* stream = fill_stream(bad_contents[i]);
        int status = board_deserialize(&board, stream);
        fclose(stream);
        assert(status == DS_ERR_CELL);
    }
}

static void test_board_check_legal(void) {
    const char* orig_board = "3 3\n"
                             "0 6. 0 0 0 0 0 0 5\n"
                             "0 0 0 0 0 0 0 0 0\n"
                             "0 0 0 0 5 0 0 0 2.\n"

                             "0 0 0 0 0 0 0 0 0\n"
                             "0 0 0 0 0 0 0 4. 0\n"
                             "0 0 0 0 0 0 0 0 0\n"

                             "0 0 0 0 0 0 5 0 0\n"
                             "0 0 0 0 0 0 0 0 0\n"
                             "0 9. 0 0 0 0 0 0 0\n";

    const char* expected1 = "----------------------------------------\n"
                            "|      6.  6*|            |          5 |\n"
                            "|            |            |            |\n"
                            "|            |      5     |          2.|\n"
                            "----------------------------------------\n"
                            "|            |            |            |\n"
                            "|            |            |      4.    |\n"
                            "|            |            |            |\n"
                            "----------------------------------------\n"
                            "|            |            |  5         |\n"
                            "|            |            |            |\n"
                            "|      9.    |            |            |\n"
                            "----------------------------------------\n";

    const char* expected2 = "----------------------------------------\n"
                            "|      6.  6*|            |          5*|\n"
                            "|            |            |            |\n"
                            "|            |      5     |          2.|\n"
                            "----------------------------------------\n"
                            "|            |            |            |\n"
                            "|            |            |      4.    |\n"
                            "|            |            |            |\n"
                            "----------------------------------------\n"
                            "|            |            |  5*        |\n"
                            "|            |            |            |\n"
                            "|      9.    |            |          5*|\n"
                            "----------------------------------------\n";

    const char* expected3 = "----------------------------------------\n"
                            "|      6.  7 |            |          5 |\n"
                            "|            |            |            |\n"
                            "|            |      5     |          2.|\n"
                            "----------------------------------------\n"
                            "|            |            |            |\n"
                            "|            |            |      4.    |\n"
                            "|            |            |            |\n"
                            "----------------------------------------\n"
                            "|            |            |  5         |\n"
                            "|            |            |            |\n"
                            "|      9.    |            |          7 |\n"
                            "----------------------------------------\n";

    int row;
    int col;

    board_t board;
    FILE* stream = fill_stream(orig_board);
    assert(board_deserialize(&board, stream) == DS_OK);
    fclose(stream);

    assert(board_is_legal(&board));

    for (row = 0; row < 9; row++) {
        for (col = 0; col < 9; col++) {
            assert(!cell_is_error(board_access(&board, row, col)));
        }
    }

    board_access(&board, 0, 2)->value = 6;
    assert(!board_is_legal(&board));

    board_mark_errors(&board);
    stream = tmpfile();
    board_print(&board, stream, TRUE);
    check_contents(stream, expected1);

    board_access(&board, 8, 8)->value = 5;
    assert(!board_is_legal(&board));

    board_mark_errors(&board);
    stream = tmpfile();
    board_print(&board, stream, TRUE);
    check_contents(stream, expected2);

    board_access(&board, 8, 8)->value = 7;
    board_access(&board, 0, 2)->value = 7;
    assert(board_is_legal(&board));

    board_mark_errors(&board);
    stream = tmpfile();
    board_print(&board, stream, TRUE);
    check_contents(stream, expected3);
}

int main() {
    test_board_access();
    test_board_print();
    test_board_serialize();
    test_board_deserialize();
    test_board_deserialize_err_fmt();
    test_board_deserialize_err_cell_val();
    test_board_check_legal();
    return 0;
}
