#include "backtrack.h"

#include "board.h"
#include <assert.h>
#include <stdio.h>

int main() {
    board_t board;
    board_init(&board, 2, 2);
    assert(num_solutions(&board) == 288); /* According to wikipedia */
    board_access(&board, 0, 0)->value = 1;
    board_access(&board, 1, 1)->value = 1;
    assert(num_solutions(&board) == 0);
    return 0;
}
