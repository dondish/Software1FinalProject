#include "lp.h"

#include "board.h"
#include <assert.h>

int main() {
    board_t board;
    lp_env_t env;

    board_init(&board, 2, 2);
    board_access(&board, 0, 0)->value = 1;
    assert(lp_env_init(&env) == LP_SUCCESS);

    lp_solve_ilp(env, &board);

    lp_env_destroy(env);

    return 0;
}
