#include "history.h"

#include "board.h"
#include "checked_alloc.h"
#include "list.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static void realloc_grow(delta_list_t* list) {
    list->capacity *= 2;
    list->deltas =
        checked_realloc(list->deltas, list->capacity * sizeof(delta_t));
}

void delta_list_init(delta_list_t* list) {
    list->size = 0;
    list->capacity = 1;
    list->deltas = checked_calloc(1, sizeof(delta_t));
}

void delta_list_destroy(delta_list_t* list) { free(list->deltas); }

void delta_list_add(delta_list_t* list, int row, int col, int old_val,
                    int new_val) {
    if (old_val == new_val) {
        return;
    }

    if (list->size == list->capacity) {
        realloc_grow(list);
    }

    list->deltas[list->size].row = row;
    list->deltas[list->size].col = col;
    list->deltas[list->size].diff = new_val - old_val;
    list->size++;
}

void delta_list_apply(board_t* board, const delta_list_t* list,
                      delta_callback_t callback) {
    int i;
    for (i = 0; i < list->size; i++) {
        cell_t* c =
            board_access(board, list->deltas[i].row, list->deltas[i].col);

        c->value += list->deltas[i].diff;

        if (callback) {
            callback(list->deltas[i].row, list->deltas[i].col,
                     c->value - list->deltas[i].diff, c->value);
        }
    }
}

void delta_list_set_diff(delta_list_t* list, const board_t* old,
                         const board_t* new) {
    int block_size = board_block_size(old);
    int row, col;

    delta_list_init(list);

    for (row = 0; row < block_size; row++) {
        for (col = 0; col < block_size; col++) {
            int old_val = board_access_const(old, row, col)->value;
            int new_val = board_access_const(new, row, col)->value;
            delta_list_add(list, row, col, old_val, new_val);
        }
    }
}

void delta_list_revert(board_t* board, const delta_list_t* list,
                       delta_callback_t callback) {
    int i;
    for (i = 0; i < list->size; i++) {
        cell_t* c =
            board_access(board, list->deltas[i].row, list->deltas[i].col);

        c->value -= list->deltas[i].diff;

        if (callback) {
            callback(list->deltas[i].row, list->deltas[i].col,
                     c->value + list->deltas[i].diff, c->value);
        }
    }
}

void history_init(history_t* history) {
    list_init(&history->list);
    history->current = NULL;
}

static void delta_list_destroy_and_free(void* item) {
    delta_list_destroy(item);
    free(item);
}

void history_destroy(history_t* history) {
    list_destroy(&history->list, delta_list_destroy_and_free);
}

void history_clear(history_t* history) {
    history_destroy(history);
    history_init(history);
}

void history_add_item(history_t* history, delta_list_t* item) {
    list_node_t* redo_start;

    delta_list_t* temp = checked_malloc(sizeof(delta_list_t));
    memcpy(temp, item, sizeof(delta_list_t));

    /* Fail fast if we attempt to reuse `item` */
    memset(item, 0, sizeof(delta_list_t));

    redo_start = history->current ? history->current->next : history->list.head;
    list_destroy_tail(&history->list, redo_start, delta_list_destroy_and_free);

    list_push(&history->list, temp);
    history->current = history->list.tail;
}

const delta_list_t* history_undo(history_t* history) {
    const delta_list_t* res;

    if (!history->current) {
        return NULL;
    }

    res = history->current->value;
    history->current = history->current->prev;
    return res;
}

const delta_list_t* history_redo(history_t* history) {
    list_node_t* next =
        history->current ? history->current->next : history->list.head;

    if (next) {
        history->current = next;
        return next->value;
    }

    return NULL;
}
