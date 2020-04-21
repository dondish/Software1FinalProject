#include "board.h"
#include "history.h"

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

int main() {
    test_delta_list_add();
    test_delta_list_apply_revert();
    return 0;
}
