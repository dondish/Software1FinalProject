#include "board.h"
#include "history.h"
#include "list.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>

static void test_delta_list_add(void) {
    delta_list_t list;

    delta_list_init(&list);
    assert(list.size == 0);

    delta_list_add(&list, 0, 3, 2, 7);
    assert(list.size == 1);
    assert(list.deltas[0].row == 0);
    assert(list.deltas[0].col == 3);
    assert(list.deltas[0].diff == 5);

    delta_list_add(&list, 1, 2, 9, 4);
    assert(list.size == 2);
    assert(list.deltas[1].row == 1);
    assert(list.deltas[1].col == 2);
    assert(list.deltas[1].diff == -5);
}

static void debug_printer_callback(int row, int col, int old_val, int new_val,
                                   void* ctx) {
    (void)ctx;
    fprintf(stderr, "(%d, %d): %d -> %d\n", row, col, old_val, new_val);
}

static void test_delta_list_apply_revert(void) {
    board_t board;
    delta_list_t delta;

    board_init(&board, 2, 2);
    board_access(&board, 0, 0)->value = 3;
    board_access(&board, 0, 2)->value = 5;

    delta_list_init(&delta);
    delta_list_add(&delta, 0, 0, 3, 7);
    delta_list_add(&delta, 0, 2, 5, 2);
    delta_list_add(&delta, 1, 3, 0, 2);

    delta_list_apply(&board, &delta, debug_printer_callback, NULL);
    assert(board_access(&board, 0, 0)->value == 7);
    assert(board_access(&board, 0, 2)->value == 2);
    assert(board_access(&board, 1, 3)->value == 2);

    delta_list_revert(&board, &delta, debug_printer_callback, NULL);
    assert(board_access(&board, 0, 0)->value == 3);
    assert(board_access(&board, 0, 2)->value == 5);
    assert(board_access(&board, 1, 3)->value == 0);

    delta_list_destroy(&delta);
}

static void test_history_add_item(void) {
    history_t history;
    delta_list_t delta;
    list_node_t* node;
    const delta_list_t* delta_ptr;

    history_init(&history);

    delta_list_init(&delta);
    delta_list_add(&delta, 2, 3, 7, 3);
    delta_list_add(&delta, 0, 0, 5, 9);
    delta_list_add(&delta, 5, 5, 8, 0);
    history_add_item(&history, &delta);

    delta_list_init(&delta);
    delta_list_add(&delta, 2, 5, 8, 9);
    delta_list_add(&delta, 3, 1, 4, 1);
    delta_list_add(&delta, 0, 0, 9, 7);
    history_add_item(&history, &delta);

    node = history.list.head;
    delta_ptr = node->value;

    assert(delta_ptr->deltas[0].row == 2);
    assert(delta_ptr->deltas[0].col == 3);
    assert(delta_ptr->deltas[0].diff == -4);
    assert(delta_ptr->deltas[1].row == 0);
    assert(delta_ptr->deltas[1].col == 0);
    assert(delta_ptr->deltas[1].diff == 4);
    assert(delta_ptr->deltas[2].row == 5);
    assert(delta_ptr->deltas[2].col == 5);
    assert(delta_ptr->deltas[2].diff == -8);

    node = node->next;
    delta_ptr = node->value;

    assert(delta_ptr->deltas[0].row == 2);
    assert(delta_ptr->deltas[0].col == 5);
    assert(delta_ptr->deltas[0].diff == 1);
    assert(delta_ptr->deltas[1].row == 3);
    assert(delta_ptr->deltas[1].col == 1);
    assert(delta_ptr->deltas[1].diff == -3);
    assert(delta_ptr->deltas[2].row == 0);
    assert(delta_ptr->deltas[2].col == 0);
    assert(delta_ptr->deltas[2].diff == -2);

    assert(node->next == NULL);
}

static void test_history_undo_redo(void) {
    history_t history;
    board_t board;
    delta_list_t delta;
    const delta_list_t* delta_ptr;

    board_init(&board, 6, 6);
    board_access(&board, 2, 3)->value = 7;
    board_access(&board, 0, 0)->value = 5;
    board_access(&board, 5, 5)->value = 8;

    history_init(&history);

    delta_list_init(&delta);
    delta_list_add(&delta, 2, 3, 7, 3);
    delta_list_add(&delta, 0, 0, 5, 9);
    delta_list_add(&delta, 5, 5, 8, 0);
    history_add_item(&history, &delta);

    delta_list_init(&delta);
    delta_list_add(&delta, 2, 5, 8, 9);
    delta_list_add(&delta, 3, 1, 4, 1);
    delta_list_add(&delta, 0, 0, 9, 7);
    history_add_item(&history, &delta);

    delta_ptr = history_undo(&history);

    assert(delta_ptr->deltas[0].row == 2);
    assert(delta_ptr->deltas[0].col == 5);
    assert(delta_ptr->deltas[0].diff == 1);
    assert(delta_ptr->deltas[1].row == 3);
    assert(delta_ptr->deltas[1].col == 1);
    assert(delta_ptr->deltas[1].diff == -3);
    assert(delta_ptr->deltas[2].row == 0);
    assert(delta_ptr->deltas[2].col == 0);
    assert(delta_ptr->deltas[2].diff == -2);

    delta_ptr = history_undo(&history);

    assert(delta_ptr->deltas[0].row == 2);
    assert(delta_ptr->deltas[0].col == 3);
    assert(delta_ptr->deltas[0].diff == -4);
    assert(delta_ptr->deltas[1].row == 0);
    assert(delta_ptr->deltas[1].col == 0);
    assert(delta_ptr->deltas[1].diff == 4);
    assert(delta_ptr->deltas[2].row == 5);
    assert(delta_ptr->deltas[2].col == 5);
    assert(delta_ptr->deltas[2].diff == -8);

    delta_ptr = history_undo(&history);

    assert(delta_ptr == NULL);

    delta_ptr = history_redo(&history);

    assert(delta_ptr->deltas[0].row == 2);
    assert(delta_ptr->deltas[0].col == 3);
    assert(delta_ptr->deltas[0].diff == -4);
    assert(delta_ptr->deltas[1].row == 0);
    assert(delta_ptr->deltas[1].col == 0);
    assert(delta_ptr->deltas[1].diff == 4);
    assert(delta_ptr->deltas[2].row == 5);
    assert(delta_ptr->deltas[2].col == 5);
    assert(delta_ptr->deltas[2].diff == -8);

    delta_ptr = history_redo(&history);

    assert(delta_ptr->deltas[0].row == 2);
    assert(delta_ptr->deltas[0].col == 5);
    assert(delta_ptr->deltas[0].diff == 1);
    assert(delta_ptr->deltas[1].row == 3);
    assert(delta_ptr->deltas[1].col == 1);
    assert(delta_ptr->deltas[1].diff == -3);
    assert(delta_ptr->deltas[2].row == 0);
    assert(delta_ptr->deltas[2].col == 0);
    assert(delta_ptr->deltas[2].diff == -2);

    delta_ptr = history_redo(&history);

    assert(delta_ptr == NULL);
}

int main() {
    test_delta_list_add();
    test_delta_list_apply_revert();
    test_history_add_item();
    test_history_undo_redo();
    return 0;
}
