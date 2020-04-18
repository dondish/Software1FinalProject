#include "backtrack.h"

#include "board.h"
#include <assert.h>
#include <stdio.h>

#define VAL(row, col) board_access(&board, row, col)->value

int main() {
    board_t board;
    board_init(&board, 2, 2);
    assert(num_solutions(&board) == 288); /* According to wikipedia */

    VAL(0, 0) = 1;
    VAL(0, 1) = 1;
    assert(num_solutions(&board) == 0);

    VAL(0, 1) = 2;
    VAL(0, 2) = 3;
    VAL(0, 3) = 4;
    VAL(1, 0) = 3;
    VAL(1, 1) = 4;
    VAL(1, 2) = 1;
    VAL(1, 3) = 2;
    VAL(2, 1) = 1;
    assert(num_solutions(&board) == 2);

    return 0;
}
