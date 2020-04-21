#include "list.h"

#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

void test_list_init() {
    list_t list;
    list_init(&list);
    assert(list.head == NULL);
    assert(list.tail == NULL);
}

static void dtor(void* value) { (void)value; }

void test_list_destroy() {
    list_t list;
    int zero = 0;
    list_init(&list);
    list_push(&list, &zero);
    list_destroy(&list, &dtor);
    assert(list.head == NULL);
    assert(list.tail == NULL);
}

void test_list_push() {
    int xers[10];
    list_t list;
    list_node_t* item;
    int i;

    list_init(&list);

    for (i = 0; i < 10; i++) {
        xers[i] = i;
        list_push(&list, &xers[i]);
    }

    assert(*(int*)list.head->value == 0);
    assert(*(int*)list.tail->value == 9);

    i = 0;
    item = list.head;
    while (item->next != NULL) {
        assert(*(int*)item->value == i);
        i++;
        item = item->next;
    }
}

void test_list_destroy_tail() {
    int xers[10];
    list_t list;
    list_node_t* item;
    int i;

    list_init(&list);

    for (i = 0; i < 10; i++) {
        xers[i] = i;
        list_push(&list, &xers[i]);
    }

    assert(*(int*)list.head->value == 0);
    assert(*(int*)list.tail->value == 9);

    item = list.head;
    for (i = 0; i < 5; i++) {
        item = item->next;
    }
    assert(*(int*)item->value == 5);
    list_destroy_tail(&list, item, &dtor);
    assert(*(int*)list.tail->value == 4);
}

void test_list_is_empty() {
    int zero = 0;
    list_t list;
    list_init(&list);
    assert(list_is_empty(&list));
    list_push(&list, &zero);
    assert(!list_is_empty(&list));
    list_destroy(&list, &dtor);
    assert(list_is_empty(&list));
}

void test_list_pop() {
    int xers[10];
    list_t list;
    int i;

    list_init(&list);

    for (i = 0; i < 10; i++) {
        xers[i] = i;
        list_push(&list, &xers[i]);
    }

    assert(*(int*)list.head->value == 0);
    assert(*(int*)list.tail->value == 9);

    i = 9;
    while (list.tail != NULL) {
        assert(*(int*)list.tail->value == i);
        list_pop(&list);
        i--;
    }
}

int main() {
    test_list_init();
    test_list_destroy();
    test_list_push();
    test_list_destroy_tail();
    test_list_is_empty();
    test_list_pop();
    return 0;
}
