#ifndef HISTORY_H
#define HISTORY_H

#include "board.h"
#include "list.h"

/**
 * Internal type representing a board delta. Users should interact with
 * `delta_list_t` and not with `delta_t` directly.
 */
typedef struct delta {
    int row;
    int col;
    int diff;
} delta_t;

/**
 * A list of simultaneous board changes. Represented in-memory as a dynamic
 * array.
 */
typedef struct delta_list {
    int size;
    int capacity;
    delta_t* deltas;
} delta_list_t;

/**
 * Represents a move history
 */
typedef struct history {
    list_t list;
    list_node_t* current;
} history_t;

/**
 * Callback invoked when applying or reverting board deltas.
 * Note that `old_val` in this context always represents the value before the
 * operation, while `new_val` represents the value after the operation.
 */
typedef void (*delta_callback_t)(int row, int col, int old_val, int new_val);

/**
 * Initialize a new, empty delta list with a capacity of 1.
 */
void delta_list_init(delta_list_t* list);

/**
 * Deallocate any memory held by `list`.
 */
void delta_list_destroy(delta_list_t* list);

/**
 * Append a delta representing a change from `old_val` to `new_val` at the
 * specified position.
 */
void delta_list_add(delta_list_t* list, int row, int col, int old_val,
                    int new_val);

/**
 * Apply the specified delta list to `board`, transitioning from old values to
 * new values. If `callback` is supplied, it is invoked
 *
 * Note: if the board contains values other than the old values supplied when
 * creating the delta list, values may be updated incorrectly.
 */
void delta_list_apply(board_t* board, const delta_list_t* list,
                      delta_callback_t callback);

/**
 * Revert the specified delta list to `board`, transitioning from new values to
 * old values.
 *
 * Note: if the board contains values other than the new values supplied when
 * creating the delta list, values may be updated incorrectly.
 */
void delta_list_revert(board_t* board, const delta_list_t* list,
                       delta_callback_t callback);

/**
 * Initialize a new, empty history.
 */
void history_init(history_t* history);

/**
 * Destroy the specified history, deallocating any memory.
 */
void history_destroy(history_t* history);

/**
 * Clear the specified history.
 */
void history_clear(history_t* history);

/**
 * Add a new item to the history, clearing the existing redo stack.
 *
 * Note: `item` is invalidated and ownership of its contents is transferred to
 * the history object. It should not be destroyed, and should not be used again
 * without being reinitialized.
 */
void history_add_item(history_t* history, delta_list_t* item);

/**
 * Move the "current move cursor" one step back, returning the original move or
 * null if there is nowhere to go.
 */
const delta_list_t* history_undo(history_t* history);

/**
 * Move the "current move cursor" one step forward, returning the new move or
 * null if there is nowhere to go.
 */
const delta_list_t* history_redo(history_t* history);

#endif
