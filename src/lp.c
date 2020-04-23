#include "lp.h"

#include "board.h"
#include "bool.h"
#include "checked_alloc.h"
#include <gurobi_c.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

bool_t lp_env_create(lp_env_t* env) {
    GRBenv* grb_env = NULL;

    if (GRBloadenv(&grb_env, NULL)) {
        return FALSE;
    }

    if (GRBsetintparam(grb_env, GRB_INT_PAR_OUTPUTFLAG, 0)) {
        GRBfreeenv(grb_env);
        return FALSE;
    }

    *env = (lp_env_t)grb_env;
    return TRUE;
}

void lp_env_free(lp_env_t env) { GRBfreeenv((GRBenv*)env); }

/**
 * Access the specified part of the variable map, based on row, column and
 * (1-based) cell value. The variable map is used to track the relationship
 * between board cells (and their values) and Gurobi variables.
 */
static int* var_map_access(int* var_map, int block_size, int row, int col,
                           int val) {
    return &var_map[block_size * (block_size * row + col) + val - 1];
}

/**
 * Set all entries of the specified variable map to -1, indicating no variable.
 */
static void clear_var_map(int* var_map, int block_size) {
    int i;
    for (i = 0; i < block_size * block_size * block_size; i++) {
        var_map[i] = -1;
    }
}

/**
 * Populate the variable map based on the specified board. Every entry of the
 * map will contain an index, or -1 if no variable is to be associated with the
 * corresponding cell value.
 */
static int compute_var_map(int* var_map, board_t* board) {
    int block_size = board_block_size(board);
    int row, col;
    int var_count = 0;

    int* candidates = checked_calloc(block_size, sizeof(int));

    clear_var_map(var_map, board_block_size(board));

    for (row = 0; row < block_size; row++) {
        for (col = 0; col < block_size; col++) {
            int candidate_count, i;

            if (!cell_is_empty(board_access(board, row, col))) {
                continue;
            }

            candidate_count =
                board_gather_candidates(board, row, col, candidates);

            for (i = 0; i < candidate_count; i++) {
                int val = candidates[i];
                *var_map_access(var_map, block_size, row, col, val) =
                    var_count++;
            }
        }
    }

    free(candidates);
    return var_count;
}

/**
 * Count the number of candidates for the specified position, based on
 * `var_map`.
 */
static int count_candidates(int* var_map, int block_size, int row, int col) {
    int count = 0;

    int val;
    for (val = 1; val <= block_size; val++) {
        if (*var_map_access(var_map, block_size, row, col, val) != -1) {
            count++;
        }
    }

    return count;
}

/*
 * Abstraction for a function that can retrieve a value from the variable map
 * based on three indices (two "contexts" and one "local offset", all in the
 * range `[0, board_size)`). This is used go generalize constraint computation
 * for cells, rows, columns and blocks.
 */
typedef int (*var_map_retriever_t)(const board_t* board, int* var_map, int ctx1,
                                   int ctx2, int local_off);

/**
 * Add constraints to the model based on variable indices found in the variable
 * map. For every values of `ctx1`, `ctx2` in `[0, block_size)`, `local_off` is
 * iterated through `[0, block_size)` as well, and all variable indices found
 * within a specific context are added as a constraint to the model.
 *
 * `indices` will be used as scratch space for the operation and should have
 * room for `block_size` entries.
 *
 * `coeffs` are the actual coefficients that will be provided to the
 * constraints, and should contain `block_size` entries. They are provided
 * externally to avoid repeated reallocation acrosss multiple `add_constraints`
 * calls.
 */
static lp_status_t add_constraints(GRBmodel* model, const board_t* board,
                                   int* var_map, var_map_retriever_t retrieve,
                                   int* indices, double* coeffs) {
    int block_size = board_block_size(board);
    int ctx1, ctx2;

    for (ctx1 = 0; ctx1 < block_size; ctx1++) {
        for (ctx2 = 0; ctx2 < block_size; ctx2++) {
            int numnz = 0;

            int local_off;
            for (local_off = 0; local_off < block_size; local_off++) {
                int var_idx = retrieve(board, var_map, ctx1, ctx2, local_off);
                if (var_idx != -1) {
                    indices[numnz++] = var_idx;
                }
            }

            if (numnz) {
                if (GRBaddconstr(model, numnz, indices, coeffs, GRB_EQUAL, 1.0,
                                 NULL)) {
                    return LP_GUROBI_ERR;
                }
            }
        }
    }

    return LP_SUCCESS;
}

/**
 * Retriever for adding cell constraints - local offset enumerates cell values.
 */
static int retrieve_by_cell(const board_t* board, int* var_map, int ctx1,
                            int ctx2, int local_off) {
    return *var_map_access(var_map, board_block_size(board), ctx1, ctx2,
                           local_off + 1);
}

/**
 * Retriever for adding row constraints - local offset enumerates columns.
 */
static int retrieve_by_row(const board_t* board, int* var_map, int ctx1,
                           int ctx2, int local_off) {
    return *var_map_access(var_map, board_block_size(board), ctx1, local_off,
                           ctx2 + 1);
}

/**
 * Retriever for adding column constraints - local offset enumerates rows.
 */
static int retrieve_by_col(const board_t* board, int* var_map, int ctx1,
                           int ctx2, int local_off) {
    return *var_map_access(var_map, board_block_size(board), local_off, ctx1,
                           ctx2 + 1);
}

/**
 * Retriever for adding block constraints - local offset enumerates index within
 * block.
 */
static int retrieve_by_block(const board_t* board, int* var_map, int ctx1,
                             int ctx2, int local_off) {
    int block_row = ctx1 / board->m;
    int block_col = ctx1 % board->m;

    int local_row = local_off / board->n;
    int local_col = local_off % board->n;

    int row = board_block_row(board, block_row, local_row);
    int col = board_block_col(board, block_col, local_col);

    return *var_map_access(var_map, board_block_size(board), row, col,
                           ctx2 + 1);
}

static void fill_coeffs(double* coeffs, int block_size) {
    int i;
    for (i = 0; i < block_size; i++) {
        coeffs[i] = 1.0;
    }
}

/**
 * Add constraints guaranteeing board validity to `model`, based on `board` and
 * the pre-computed `var_map`.
 */
static lp_status_t add_board_constraints(GRBmodel* model, const board_t* board,
                                         int* var_map) {
    lp_status_t ret = LP_SUCCESS;

    int block_size = board_block_size(board);

    int* indices = checked_calloc(block_size, sizeof(int));
    double* coeffs = checked_calloc(block_size, sizeof(double));

    fill_coeffs(coeffs, block_size);

    ret = add_constraints(model, board, var_map, retrieve_by_cell, indices,
                          coeffs);
    if (ret != LP_SUCCESS) {
        goto cleanup;
    }

    ret = add_constraints(model, board, var_map, retrieve_by_row, indices,
                          coeffs);
    if (ret != LP_SUCCESS) {
        goto cleanup;
    }

    ret = add_constraints(model, board, var_map, retrieve_by_col, indices,
                          coeffs);
    if (ret != LP_SUCCESS) {
        goto cleanup;
    }

    ret = add_constraints(model, board, var_map, retrieve_by_block, indices,
                          coeffs);

cleanup:
    free(coeffs);
    free(indices);
    return ret;
}

/**
 * Add variables of the specified type to `model` based on `var_map`,
 * constraining them to `[0, 1]`. The objective function will be set to favor
 * placing values in cells with fewer candidates, which has the effect of making
 * the model more "confident" about values in cells with few candidates.
 */
static lp_status_t add_vars(GRBmodel* model, int block_size, int* var_map,
                            char var_type) {
    int row;
    int col;

    for (row = 0; row < block_size; row++) {
        for (col = 0; col < block_size; col++) {
            int candidate_count =
                count_candidates(var_map, block_size, row, col);

            int i;
            for (i = 0; i < candidate_count; i++) {
                /* Weight each variable for this cell with `candidate_count` in
                 * the objective function, which, as we are minimizing, will
                 * cause Gurobi to favor cells with fewer candidates. */
                if (GRBaddvar(model, 0, NULL, NULL, candidate_count, 0.0, 1.0,
                              var_type, NULL)) {
                    return LP_GUROBI_ERR;
                }
            }
        }
    }

    if (GRBsetintattr(model, GRB_INT_ATTR_MODELSENSE, GRB_MINIMIZE)) {
        return LP_GUROBI_ERR;
    }

    if (GRBupdatemodel(model)) {
        return LP_GUROBI_ERR;
    }

    return LP_SUCCESS;
}

/**
 * Callback invoked with nonzero LP values on success.
 */
typedef void (*lp_val_callback_t)(int block_size, int row, int col, int val,
                                  double score, void* ctx);

/**
 * Create a new sudoku model in the specified environment.
 */
static bool_t create_model(lp_env_t env, GRBmodel** model) {
    return !GRBnewmodel((GRBenv*)env, model, "sudoku", 0, NULL, NULL, NULL,
                        NULL, NULL);
}

/**
 * Report nonzero variable values from `model` to `callback`.
 */
static lp_status_t report_var_values(GRBmodel* model, int block_size,
                                     int* var_map, int var_count,
                                     lp_val_callback_t callback,
                                     void* callback_ctx) {
    lp_status_t ret = LP_SUCCESS;

    double* var_values = checked_calloc(var_count, sizeof(double));
    int row, col, val;

    if (GRBgetdblattrarray(model, GRB_DBL_ATTR_X, 0, var_count, var_values)) {
        ret = LP_GUROBI_ERR;
        goto cleanup;
    }

    for (row = 0; row < block_size; row++) {
        for (col = 0; col < block_size; col++) {
            for (val = 1; val <= block_size; val++) {
                int var_idx =
                    *var_map_access(var_map, block_size, row, col, val);

                if (var_idx != -1) {
                    double score = var_values[var_idx];
                    if (score > 0.0) {
                        callback(block_size, row, col, val, score,
                                 callback_ctx);
                    }
                }
            }
        }
    }

cleanup:
    free(var_values);
    return ret;
}

/**
 * Solve `board` using LP with the specified variable types, reporting
 * values to `callback` on success.
 */
static lp_status_t lp_solve(lp_env_t env, board_t* board, char var_type,
                            lp_val_callback_t callback, void* callback_ctx) {
    lp_status_t ret = LP_SUCCESS;

    int block_size = board_block_size(board);

    GRBmodel* model = NULL;
    int* var_map;
    int var_count;

    int optim_status;

    if (!create_model(env, &model)) {
        return LP_GUROBI_ERR;
    }

    var_map =
        checked_malloc(block_size * block_size * block_size * sizeof(int));
    var_count = compute_var_map(var_map, board);

    ret = add_vars(model, block_size, var_map, var_type);
    if (ret != LP_SUCCESS) {
        goto cleanup;
    }

    ret = add_board_constraints(model, board, var_map);
    if (ret != LP_SUCCESS) {
        goto cleanup;
    }

    if (GRBoptimize(model)) {
        ret = LP_GUROBI_ERR;
        goto cleanup;
    }

    if (GRBgetintattr(model, GRB_INT_ATTR_STATUS, &optim_status)) {
        ret = LP_GUROBI_ERR;
        goto cleanup;
    }

    if (optim_status == GRB_OPTIMAL) {
        ret = report_var_values(model, block_size, var_map, var_count, callback,
                                callback_ctx);
    } else if (optim_status == GRB_INFEASIBLE ||
               optim_status == GRB_INF_OR_UNBD) {
        ret = LP_INFEASIBLE;
    } else {
        ret = LP_GUROBI_ERR;
    }

cleanup:
    free(var_map);
    GRBfreemodel(model);
    return ret;
}

/* ILP */

/**
 * Value callback used when validating with ILP.
 */
static void ilp_validate_callback(int block_size, int row, int col, int val,
                                  double score, void* ctx) {
    (void)block_size;
    (void)row;
    (void)col;
    (void)val;
    (void)score;
    (void)ctx;
}

lp_status_t lp_validate_ilp(lp_env_t env, board_t* board) {
    return lp_solve(env, board, GRB_BINARY, ilp_validate_callback, NULL);
}

/**
 * Value callback used when solving with ILP.
 */
static void ilp_solve_callback(int block_size, int row, int col, int val,
                               double score, void* ctx) {
    board_t* board = ctx;

    (void)block_size;
    (void)score;
    board_access(board, row, col)->value = val;
}

lp_status_t lp_solve_ilp(lp_env_t env, board_t* board) {
    return lp_solve(env, board, GRB_BINARY, ilp_solve_callback, board);
}

/* Continuous LP */

void lp_cell_candidates_destroy(lp_cell_candidates_t* candidates) {
    free(candidates->candidates);
    memset(candidates, 0, sizeof(lp_cell_candidates_t));
}

static void candidates_realloc_grow(lp_cell_candidates_t* candidates) {
    candidates->capacity = candidates->capacity * 2 + 1;
    candidates->candidates = checked_realloc(
        candidates->candidates, candidates->capacity * sizeof(lp_candidate_t));
}

/**
 * Append the specified scored value to the candidate list.
 */
static void candidates_add(lp_cell_candidates_t* candidates, int val,
                           double score) {
    if (candidates->size == candidates->capacity) {
        candidates_realloc_grow(candidates);
    }

    candidates->candidates[candidates->size].val = val;
    candidates->candidates[candidates->size].score = score;

    candidates->size++;
}

static void continuous_val_callback(int block_size, int row, int col, int val,
                                    double score, void* ctx) {
    lp_cell_candidates_t* candidate_board = ctx;
    candidates_add(&candidate_board[row * block_size + col], val, score);
}

lp_status_t lp_solve_continuous(lp_env_t env, board_t* board,
                                lp_cell_candidates_t* candidate_board) {
    int block_size = board_block_size(board);

    memset(candidate_board, 0,
           sizeof(lp_cell_candidates_t) * block_size * block_size);

    return lp_solve(env, board, GRB_CONTINUOUS, continuous_val_callback,
                    candidate_board);
}
