#ifndef LP_H
#define LP_H

#include "board.h"

typedef enum lp_status { LP_SUCCESS, LP_INFEASIBLE, LP_GUROBI_ERR } lp_status_t;

lp_status_t solve_ilp(board_t* board);

#endif
