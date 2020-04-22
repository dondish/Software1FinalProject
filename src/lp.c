#include "lp.h"

#include "board.h"
#include "bool.h"
#include "checked_alloc.h"
#include <gurobi_c.h>
#include <stddef.h>
#include <stdlib.h>

static bool_t create_model(lp_env_t env, GRBmodel** model) {
    return !GRBnewmodel(env, model, "sudoku", 0, NULL, NULL, NULL, NULL, NULL);
}

bool_t lp_env_init(lp_env_t* env) {
    GRBenv* grb_env = NULL;
    int err = GRBloadenv(&grb_env, NULL);
    if (err) {
        return LP_GUROBI_ERR;
    }
    *env = grb_env;
    return LP_SUCCESS;
}

void lp_env_destroy(lp_env_t env) { GRBfreeenv((GRBenv*)env); }

static int gather_candidates(board_t* board, int row, int col,
                             int* candidates) {
    cell_t* cell = board_access(board, row, col);

    int block_size = board_block_size(board);
    int candidate_count = 0;

    int val;
    for (val = 1; val <= block_size; val++) {
        cell->value = val;
        if (board_is_legal(board)) {
            candidates[candidate_count++] = val;
        }
    }

    cell->value = 0;

    return candidate_count;
}

static void clear_var_map(int* var_map, int block_size) {
    int i;
    for (i = 0; i < block_size * block_size * block_size; i++) {
        var_map[i] = -1;
    }
}

static int* var_map_access(int* var_map, int block_size, int row, int col,
                           int val) {
    return &var_map[block_size * (block_size * row + col) + val - 1];
}

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

            candidate_count = gather_candidates(board, row, col, candidates);

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

static void fill_coeffs(double* coeffs, int block_size) {
    int i;
    for (i = 0; i < block_size; i++) {
        coeffs[i] = 1.0;
    }
}

static lp_status_t add_constraints(GRBmodel* model, board_t* board,
                                   int* var_map) {
    lp_status_t ret = LP_SUCCESS;

    int block_size = board_block_size(board);

    int* indices = checked_calloc(block_size, sizeof(int));
    double* coeffs = checked_calloc(block_size, sizeof(double));
    int row, col, val;

    fill_coeffs(coeffs, block_size);

    for (row = 0; row < block_size; row++) {
        for (col = 0; col < block_size; col++) {
            int numnz = 0;

            for (val = 1; val <= block_size; val++) {
                int var_idx =
                    *var_map_access(var_map, block_size, row, col, val);
                if (var_idx != -1) {
                    indices[numnz++] = var_idx;
                }
            }

            if (numnz) {
                if (GRBaddconstr(model, numnz, indices, coeffs, GRB_EQUAL, 1.0,
                                 NULL)) {
                    ret = LP_GUROBI_ERR;
                    goto cleanup;
                }
            }
        }
    }

    for (val = 1; val <= block_size; val++) {
        for (row = 0; row < block_size; row++) {
            int numnz = 0;

            for (col = 0; col < block_size; col++) {
                int var_idx =
                    *var_map_access(var_map, block_size, row, col, val);
                if (var_idx != -1) {
                    indices[numnz++] = var_idx;
                }
            }

            if (numnz) {
                if (GRBaddconstr(model, numnz, indices, coeffs, GRB_EQUAL, 1.0,
                                 NULL)) {
                    ret = LP_GUROBI_ERR;
                    goto cleanup;
                }
            }
        }
    }

    for (val = 1; val <= block_size; val++) {
        for (col = 0; col < block_size; col++) {
            int numnz = 0;

            for (row = 0; row < block_size; row++) {
                int var_idx =
                    *var_map_access(var_map, block_size, row, col, val);
                if (var_idx != -1) {
                    indices[numnz++] = var_idx;
                }
            }

            if (numnz) {
                if (GRBaddconstr(model, numnz, indices, coeffs, GRB_EQUAL, 1.0,
                                 NULL)) {
                    ret = LP_GUROBI_ERR;
                    goto cleanup;
                }
            }
        }
    }

    for (val = 1; val <= block_size; val++) {
        int block_row, block_col, local_row, local_col;
        for (block_row = 0; block_row < board->n; block_row++) {
            for (block_col = 0; block_col < board->m; block_col++) {
                int numnz = 0;
                for (local_row = 0; local_row < board->m; local_row++) {
                    for (local_col = 0; local_col < board->n; local_col++) {
                        int var_idx = *var_map_access(
                            var_map, block_size,
                            block_row * board->m + local_row,
                            block_col * board->n + local_col, val);
                        if (var_idx != -1) {
                            indices[numnz++] = var_idx;
                        }
                    }
                }

                if (numnz) {
                    if (GRBaddconstr(model, numnz, indices, coeffs, GRB_EQUAL,
                                     1.0, NULL)) {
                        ret = LP_GUROBI_ERR;
                        goto cleanup;
                    }
                }
            }
        }
    }

cleanup:
    free(coeffs);
    free(indices);
    return ret;
}

lp_status_t lp_solve_ilp(lp_env_t env, board_t* board) {
    lp_status_t ret = LP_SUCCESS;

    int block_size = board_block_size(board);

    GRBmodel* model = NULL;
    int var_count, var_idx;
    int* var_map;

    int optim_status;

    if (!create_model(env, &model)) {
        return LP_GUROBI_ERR;
    }

    var_map =
        checked_malloc(block_size * block_size * block_size * sizeof(int));
    var_count = compute_var_map(var_map, board);

    for (var_idx = 0; var_idx < var_count; var_idx++) {
        if (GRBaddvar(model, 0, NULL, NULL, 0.0, 0.0, 1.0, GRB_BINARY, NULL)) {
            ret = LP_GUROBI_ERR;
            goto cleanup;
        }
    }

    if (GRBupdatemodel(model)) {
        ret = LP_GUROBI_ERR;
        goto cleanup;
    }

    ret = add_constraints(model, board, var_map);
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
        double* var_values = checked_calloc(var_count, sizeof(double));
        int row, col, val;

        if (GRBgetdblattrarray(model, GRB_DBL_ATTR_X, 0, var_count,
                               var_values)) {
            ret = LP_GUROBI_ERR;
            goto cleanup;
        }

        for (row = 0; row < block_size; row++) {
            for (col = 0; col < block_size; col++) {
                for (val = 1; val <= block_size; val++) {
                    int var_idx =
                        *var_map_access(var_map, block_size, row, col, val);
                    if (var_idx != -1 && var_values[var_idx] > 0.0) {
                        board_access(board, row, col)->value = val;
                    }
                }
            }
        }

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
