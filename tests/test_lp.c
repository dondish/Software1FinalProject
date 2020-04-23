#include "lp.h"

#include "board.h"
#include <assert.h>
#include <stdio.h>

#define VAL(row, col) board_access(&board, row, col)->value

int main() {
    board_t board;
    lp_env_t env;
    int block_size;
    int i, j;

    lp_cell_candidates_t candidate_board[81];

    board_init(&board, 3, 3);
    block_size = board_block_size(&board);

    VAL(0, 0) = 1;
    VAL(5, 7) = 3;

    assert(lp_env_create(&env));

    assert(lp_solve_continuous(env, &board, candidate_board) == LP_SUCCESS);
    for (i = 0; i < block_size; i++) {
        for (j = 0; j < block_size; j++) {
            int k;
            lp_cell_candidates_t* candidates =
                &candidate_board[i * block_size + j];

            for (k = 0; k < candidates->size; k++) {
                fprintf(stderr, "(%d, %d): %d (%f)\n", i, j,
                        candidates->candidates[k].val,
                        candidates->candidates[k].score);
            }
        }
    }

    assert(lp_validate_ilp(env, &board) == LP_SUCCESS);
    assert(lp_solve_ilp(env, &board) == LP_SUCCESS);

    board_print(&board, stderr, FALSE);
    assert(board_is_legal(&board));

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

    assert(lp_validate_ilp(env, &board) == LP_INFEASIBLE);
    assert(lp_solve_ilp(env, &board) == LP_INFEASIBLE);

    VAL(1, 3) = 3;
    VAL(2, 0) = 2;
    VAL(2, 1) = 3;
    VAL(2, 2) = 4;
    VAL(3, 0) = 4;
    VAL(3, 1) = 3;
    VAL(3, 2) = 2;
    VAL(3, 3) = 1;

    board_print(&board, stderr, FALSE);
    assert(!board_is_legal(&board));

    /* The solver has nothing to do here, so it will "succeed" */
    assert(lp_validate_ilp(env, &board) == LP_SUCCESS);
    assert(lp_solve_ilp(env, &board) == LP_SUCCESS);

    lp_env_free(env);

    return 0;
}
