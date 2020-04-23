#include "lp.h"

#include "board.h"
#include "bool.h"
#include "checked_alloc.h"
#include <gurobi_c.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define GENERATE_MAX_ATTEMPTS 1000

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
 * If an empty cell without candidates is found, false will be returned (the
 * board is unsolvable).
 */
static bool_t compute_var_map(int* var_map, int* var_count, board_t* board) {
    bool_t ret = TRUE;

    int block_size = board_block_size(board);
    int row, col;
    int count = 0;

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

            if (!candidate_count) {
                ret = FALSE;
                goto cleanup;
            }

            for (i = 0; i < candidate_count; i++) {
                int val = candidates[i];
                *var_map_access(var_map, block_size, row, col, val) = count++;
            }
        }
    }

    *var_count = count;

cleanup:
    free(candidates);
    return ret;
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

    if (!compute_var_map(var_map, &var_count, board)) {
        ret = LP_INFEASIBLE;
        goto cleanup;
    }

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

/* Puzzle Generation */

/**
 * Store the indices of all empty cells in `board` to `cell_indices`, returning
 * the number of such cells found.
 */
static int get_empty_cells(const board_t* board, int* cell_indices) {
    int block_size = board_block_size(board);

    int count = 0;

    int cell_idx;
    for (cell_idx = 0; cell_idx < block_size * block_size; cell_idx++) {
        if (cell_is_empty(&board->cells[cell_idx])) {
            cell_indices[count++] = cell_idx;
        }
    }

    return count;
}

static void shuffle(int* arr, int size) {
    int i;
    for (i = 0; i < size - 1; i++) {
        int remaining = size - i;
        int j = rand() % remaining;

        int temp;
        temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

/**
 * Set the specified cell of `board` to a random legal candidate, returning
 * false if no candidates exist.
 */
static bool_t set_random_candidate(board_t* board, int idx) {
    int ret = TRUE;

    int block_size = board_block_size(board);
    int* candidates = checked_calloc(block_size, sizeof(int));

    int row = idx / block_size;
    int col = idx % block_size;

    int candidate_count = board_gather_candidates(board, row, col, candidates);
    if (!candidate_count) {
        ret = FALSE;
        goto cleanup;
    }

    board->cells[idx].value = candidates[rand() % candidate_count];

cleanup:
    free(candidates);
    return ret;
}

/**
 * Attempt a single iteration of the ILP generation algorithm.
 */
static lp_status_t try_do_gen(lp_env_t env, board_t* board,
                              int* empty_cell_indices, int empty_cell_count,
                              int add) {
    int i;

    shuffle(empty_cell_indices, empty_cell_count);

    for (i = 0; i < add; i++) {
        if (!set_random_candidate(board, empty_cell_indices[i])) {
            return LP_INFEASIBLE;
        }
    }

    return lp_solve_ilp(env, board);
}

/**
 * Clear `count` random cells from `board`.
 */
static void clear_random_cells(board_t* board, int count) {
    int block_size = board_block_size(board);
    int board_size = block_size * block_size;

    int* cell_indices = checked_calloc(block_size * block_size, sizeof(int));

    int i;
    for (i = 0; i < board_size; i++) {
        cell_indices[i] = i;
    }

    shuffle(cell_indices, board_size);

    for (i = 0; i < count; i++) {
        board->cells[cell_indices[i]].value = 0;
    }

    free(cell_indices);
}

lp_gen_status_t lp_gen_ilp(lp_env_t env, board_t* board, int add, int leave) {
    lp_gen_status_t ret = GEN_MAX_ATTEMPTS;

    int block_size = board_block_size(board);

    int* empty_cell_indices =
        checked_calloc(block_size * block_size, sizeof(int));
    int empty_cell_count = get_empty_cells(board, empty_cell_indices);

    int iter;

    if (empty_cell_count < add) {
        ret = GEN_TOO_FEW_EMPTY;
        goto cleanup;
    }

    for (iter = 0; iter < GEN_MAX_ATTEMPTS; iter++) {
        lp_status_t attempt_status =
            try_do_gen(env, board, empty_cell_indices, empty_cell_count, add);

        if (attempt_status == LP_SUCCESS) {
            ret = GEN_SUCCESS;
            break;
        }

        if (attempt_status == LP_GUROBI_ERR) {
            ret = GEN_GUROBI_ERR;
            break;
        }
    }

    clear_random_cells(board, block_size * block_size - leave);

cleanup:
    free(empty_cell_indices);
    return ret;
}

/* Continuous LP */

void lp_cell_candidates_destroy(lp_cell_candidates_t* candidates) {
    free(candidates->candidates);
    memset(candidates, 0, sizeof(lp_cell_candidates_t));
}

void lp_cell_candidates_array_destroy(lp_cell_candidates_t* candidates,
                                      int block_size) {
    int i, j;
    for (i = 0; i < block_size; i++) {
        for (j = 0; j < block_size; j++) {
            lp_cell_candidates_destroy(&candidates[i * block_size + j]);
        }
    }
    free(candidates);
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

/**
 * Check whether `val` is legal for `cell`.
 */
static bool_t is_value_legal(board_t* board, cell_t* cell, int val) {
    int old_val = cell->value;
    bool_t ret;

    cell->value = val;
    ret = board_is_legal(board);
    cell->value = old_val;

    return ret;
}

/**
 * Select a random, legal candidate from `candidates` whose score is at
 * least `thresh`. The probability of each candidate being drawn is
 * proportional to its score.
 */
static lp_candidate_t* random_select(lp_cell_candidates_t* candidates,
                                     board_t* board, cell_t* cell,
                                     double thresh) {
    lp_candidate_t* ret = NULL;

    double total_score = 0;

    double* cumulative_scores;

    /* These point into the list of candidates */
    lp_candidate_t** viable_candidates;
    int viable_count = 0;

    double cumulative_threshold;
    int i = 0;

    if (!candidates->size) {
        return NULL;
    }

    cumulative_scores = checked_calloc(candidates->size, sizeof(double));
    viable_candidates =
        checked_calloc(candidates->size, sizeof(lp_candidate_t*));

    for (; i < candidates->size; i++) {
        lp_candidate_t* can = &candidates->candidates[i];

        if (can->score < thresh || !is_value_legal(board, cell, can->val)) {
            continue;
        }

        /* This candidate is legal, remember it. */
        viable_candidates[viable_count] = can;
        cumulative_scores[viable_count] =
            (viable_count == 0 ? 0 : cumulative_scores[viable_count - 1]) +
            can->score;
        viable_count++;
    }

    if (!viable_count) {
        goto cleanup;
    }

    total_score = cumulative_scores[viable_count - 1];

    /* In order to select based on score, begin by selecting a threshold in
     * [0, total_score]...
     */
    cumulative_threshold = (rand() / (double)RAND_MAX) * total_score;

    /* ...and find the first range that contains it (this loop is correct as
     * `cumulative_scores` are strictly increasing and we know that
     * `cumulative_threshold` is bounded by the last one). */
    i = 0;
    while (cumulative_threshold > cumulative_scores[i]) {
        i++;
    }

    ret = viable_candidates[i];

cleanup:
    free(viable_candidates);
    free(cumulative_scores);
    return ret;
}

lp_status_t lp_guess_continuous(lp_env_t env, board_t* board, double thresh) {
    int block_size = board_block_size(board);
    int row;
    int col;

    lp_cell_candidates_t* candidate_board =
        checked_calloc(block_size * block_size, sizeof(lp_cell_candidates_t));

    lp_status_t status = lp_solve_continuous(env, board, candidate_board);

    if (status != LP_SUCCESS)
        goto cleanup;

    for (row = 0; row < block_size; row++) {
        for (col = 0; col < block_size; col++) {
            lp_candidate_t* can =
                random_select(&candidate_board[row * block_size + col], board,
                              board_access(board, row, col), thresh);
            if (can) {
                board_access(board, row, col)->value = can->val;
            }
        }
    }

cleanup:
    lp_cell_candidates_array_destroy(candidate_board, block_size);
    return status;
}
