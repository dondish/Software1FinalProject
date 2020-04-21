#ifndef LP_H
#define LP_H

#include "board.h"
#include "bool.h"

/**
 * Opaque type representing a linear programming environment.
 */
typedef void* lp_env_t;

/**
 * Linear programming status codes
 */
typedef enum lp_status {
    LP_SUCCESS,    /* Solving succeeded */
    LP_INFEASIBLE, /* Board is infeasible */
    LP_GUROBI_ERR  /* Internal gurobi error */
} lp_status_t;

/**
 * Initialize a new linear programming environment.
 */
bool_t lp_env_init(lp_env_t* env);

/**
 * Destroy a linear programming environment.
 */
void lp_env_destroy(lp_env_t env);

/**
 * Attempt to solve `board` in-place using ILP
 */
lp_status_t lp_solve_ilp(lp_env_t env, board_t* board);

#endif
