#include "lp.h"

#include "board.h"
#include <assert.h>

#define VAL(row, col) board_access(&board, row, col)->value

int main() {
    board_t board;
    lp_env_t env;
    int block_size;
    int i, j;

    board_init(&board, 4, 4);
    board_access(&board, 0, 0)->value = 1;
    assert(lp_env_init(&env) == LP_SUCCESS);

    assert(lp_solve_ilp(env, &board) == LP_SUCCESS);

    board_print(&board, stderr, FALSE);
    assert(board_is_legal(&board));

    block_size = board_block_size(&board);
    for (i = 0; i < block_size; i++) {
        for (j = 0; j < block_size; j++) {
            assert(!cell_is_empty(board_access(&board, i, j)));
        }
    }

    board_destroy(&board);
    board_init(&board, 2, 2);

    VAL(0, 0) = 1;
    VAL(0, 1) = 2;
    VAL(1, 0) = 3;
    VAL(1, 1) = 4;
    VAL(0, 2) = 3;
    VAL(0, 3) = 4;
    VAL(1, 2) = 1;
    VAL(2, 3) = 2;

    board_print(&board, stderr, FALSE);
    assert(board_is_legal(&board));

    assert(lp_solve_ilp(env, &board) == LP_INFEASIBLE);

    lp_env_destroy(env);

    return 0;
}
