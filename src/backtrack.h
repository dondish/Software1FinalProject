/**
 * backtrack.h - Exhaustive backtracking implementation.
 */

#ifndef BACKTRACK_H
#define BACKTRACK_H

#include "board.h"

/**
 * Use exhaustive backtracking to find the number of solutions to `board`.
 *
 * Note that the board's contents may be written to, but will be restored before
 * the function returns.
 */
int num_solutions(board_t* board);

#endif
