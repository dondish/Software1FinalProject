#include "backtrack.h"

#include "board.h"
#include "bool.h"
#include "checked_alloc.h"
#include "list.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    int idx;
    int value;
} backtrack_state_t;

static void backtrack_state_push(list_t* stack,
                                 const backtrack_state_t* state) {
    void* heap_state = checked_malloc(sizeof(backtrack_state_t));
    memcpy(heap_state, state, sizeof(backtrack_state_t));
    list_push(stack, heap_state);
}

static backtrack_state_t* backtrack_state_top(list_t* stack) {
    return stack->tail->value;
}

static bool_t advance_to_nonempty(const board_t* board, int* idx) {
    int block_size = board_block_size(board);

    while (cell_is_empty(&board->cells[*idx])) {
        (*idx)++;
        if (*idx == block_size * block_size) {
            return FALSE;
        }
    }

    return TRUE;
}

int num_solutions(board_t* board) {
    list_t stack;

    int block_size = board_block_size(board);

    int count = 0;
    int idx = 0;

    list_init(&stack);

    if (advance_to_nonempty(board, &idx)) {
        backtrack_state_t init;
        init.idx = idx;
        init.value = 0; /* `value` is incremented on every iteration, so start
                           before the minimal one. */
        backtrack_state_push(&stack, &init);
    }

    while (!list_is_empty(&stack)) {
        backtrack_state_t* state = backtrack_state_top(&stack);
        int value = state->value;

        do {
            value++;
            if (value > block_size) {
                break;
            }

            board->cells[state->idx].value = value;
        } while (!board_check_legal(board));

        if (value > block_size) {
            /* We've exhausted all possibilities for this cell - reset it
             * and return to the previous one. */
            board->cells[state->idx].value = 0;
            list_pop(&stack);
        } else {
            int next_idx = state->idx;

            if (advance_to_nonempty(board, &next_idx)) {
                /* We still have more empty cells to explore.  */
                backtrack_state_t next;
                next.idx = next_idx;
                next.value = 0;
                backtrack_state_push(&stack, &next);
            } else {
                /* We've finished the board - record our success! */
                count++;
            }
        }
    }

    return count;
}
