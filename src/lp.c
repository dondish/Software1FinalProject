#include "lp.h"

#include "board.h"
#include "bool.h"
#include "checked_alloc.h"
#include <gurobi_c.h>
#include <stddef.h>
#include <stdio.h>
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
    return &var_map[block_size * (block_size * row + col) + val];
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

lp_status_t lp_solve_ilp(lp_env_t env, board_t* board) {
    lp_status_t ret = LP_SUCCESS;

    int block_size = board_block_size(board);

    GRBmodel* model = NULL;
    int var_count, var_idx;
    int* var_map;

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

cleanup:
    free(var_map);
    return ret;
}
