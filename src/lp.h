/**
 * lp.h - Linear programming solvers and validators.
 */

#ifndef LP_H
#define LP_H

#include "board.h"
#include "bool.h"

/**
 * Opaque type representing a linear programming environment.
 */
typedef struct lp_env_impl* lp_env_t;

/**
 * Linear programming status codes.
 */
typedef enum lp_status {
    LP_SUCCESS,    /* Solving succeeded */
    LP_INFEASIBLE, /* Board is infeasible */
    LP_GUROBI_ERR  /* Internal gurobi error */
} lp_status_t;

/**
 * Status codes for ILP-based puzzle generator.
 */
typedef enum lp_gen_status {
    GEN_SUCCESS,       /* Puzzle generation succeeded */
    GEN_TOO_FEW_EMPTY, /* To few cells on the board were empty */
    GEN_MAX_ATTEMPTS,  /* The generator was unable to generate a puzzle after
                             1000 attempts */
    GEN_GUROBI_ERR     /* Internal gurobi error */
} lp_gen_status_t;

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
bool_t lp_env_create(lp_env_t* env);

/**
 * Destroy a linear programming environment.
 */
void lp_env_free(lp_env_t env);

/**
 * Validate `board` using ILP.
 *
 * Note: this function does not check the legality of the board, meaning that
 * it may still report success when called on an erroneous board (when it has no
 * conflicting cells to fill in itself).
 */
lp_status_t lp_validate_ilp(lp_env_t env, board_t* board);

/**
 * Attempt to solve `board` in-place using ILP.
 *
 * Note: this function does not check the legality of the board, meaning that
 * it may still report success when called on an erroneous board (when it has no
 * conflicting cells to fill in itself).
 */
lp_status_t lp_solve_ilp(lp_env_t env, board_t* board);

/**
 * Attempt to generate a puzzle in `board` by filling `add` empty cells with
 * random legal values, using the ILP solver to solve it, and clearing `remove`
 * cells. If, after 1000 attempts, the process fails, `GEN_MAX_ATTEMTPS` is
 * returned.
 */
lp_gen_status_t lp_gen_ilp(lp_env_t env, board_t* board, int add, int remove);

/**
 * Deallocate any memory held by the specified candidate list.
 */
void lp_cell_candidates_destroy(lp_cell_candidates_t* candidates);

/**
 * Deallocate any memory held by each candidate list in the array and then the array itself.
 */
void lp_cell_candidates_array_destroy(lp_cell_candidates_t* candidates, int block_size);

/**
 * Use continuous LP to search for solutions to `board`, storing scored
 * candidates in `candidate_board`. The size of `candidate_board` should be that
 * of the board.
 *
 * Note: this function does not check the legality of the board, meaning that
 * it may still report success when called on an erroneous board (when it has no
 * conflicting cells to fill in itself).
 */
lp_status_t lp_solve_continuous(lp_env_t env, board_t* board,
                                lp_cell_candidates_t* candidate_board);

/**
 * Guess a solution to the board by running continuous LP on it and filling in
 * cells that have values with score above `thresh`.
 */
lp_status_t lp_guess_continuous(lp_env_t env, board_t* board, double thresh);

#endif
