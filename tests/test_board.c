#include "board.h"

#include <assert.h>

void test_board_access() {
    board_t board;

    assert(board_init(&board, 2, 5));
    assert(board_block_size(&board) == 10);

    assert(board.cells[13].value == 0);
    board_access(&board, 1, 3)->value = 17;
    assert(board.cells[13].value == 17);
}

int main() {
    test_board_access();
    return 0;
}
