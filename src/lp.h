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
 * Represents a scored candidate for a specific cell value.
 */
typedef struct lp_candidate {
    int val;
    double score;
} lp_candidate_t;

/**
 * Represents a list of scored candidate values for a given cell.
 */
typedef struct lp_cell_candidates {
    int size;
    int capacity;
    lp_candidate_t* candidates;
} lp_cell_candidates_t;

/**
 * Initialize a new linear programming environment.
 */
bool_t lp_env_init(lp_env_t* env);

/**
 * Destroy a linear programming environment.
 */
void lp_env_destroy(lp_env_t env);

/**
 * Attempt to solve `board` in-place using ILP.
 *
 * Note: this function does not check the legality of the board, meaning that
 * the it may still report success when called on an erroneous board (when it
 * has no conflicting cells to fill in itself).
 */
lp_status_t lp_solve_ilp(lp_env_t env, board_t* board);

/**
 * Deallocate any memory held by the specified candidate list.
 */
void lp_cell_candidates_destroy(lp_cell_candidates_t* candidates);

/**
 * Use continuous LP to search for solutions to `board`, storing scored
 * candidates in `candidate_board`. The size of `candidate_board` should be that
 * of the board.
 *
 * Note: this function does not check the legality of the board, meaning that
 * the it may still report success when called on an erroneous board (when it
 * has no conflicting cells to fill in itself).
 */
lp_status_t lp_solve_continuous(lp_env_t env, board_t* board,
                                lp_cell_candidates_t* candidate_board);

#endif
