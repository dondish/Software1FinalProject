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

int main() {
    test_list_init();
    test_list_destroy();
    test_list_push();
    return 0;
}
