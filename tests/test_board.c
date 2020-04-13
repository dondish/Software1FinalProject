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

static void test_board_print_empty() {
    board_t board;
    FILE* stream;
    char buf[1024] = {0};
    const char* expected = "-------------------------------------------\n"
                           "|                    |                    |\n"
                           "|                    |                    |\n"
                           "-------------------------------------------\n"
                           "|                    |                    |\n"
                           "|                    |                    |\n"
                           "-------------------------------------------\n"
                           "|                    |                    |\n"
                           "|                    |                    |\n"
                           "-------------------------------------------\n"
                           "|                    |                    |\n"
                           "|                    |                    |\n"
                           "-------------------------------------------\n"
                           "|                    |                    |\n"
                           "|                    |                    |\n"
                           "-------------------------------------------\n";

    assert(board_init(&board, 2, 5));
    stream = tmpfile();
    assert(stream);

    board_print(&board, stream);
    rewind(stream);
    fread(buf, 1, sizeof(buf), stream);
    fputs(buf, stderr);

    assert(!strcmp(buf, expected));
}

int main() {
    test_board_access();
    test_board_print_empty();
    return 0;
}
