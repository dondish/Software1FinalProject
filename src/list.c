#include "list.h"

#include "bool.h"
#include "checked_alloc.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

void list_init(list_t* list) {
    list->head = NULL;
    list->tail = NULL;
}

void list_destroy(list_t* list, list_item_dtor_t dtor) {
    list_node_t* iter = list->head;

    while (iter) {
        list_node_t* next = iter->next;
        dtor(iter->value);
        free(iter);
        iter = next;
    }

    /* Fail fast if we accidentally attempt to use the list later. */
    memset(list, 0, sizeof(list_t));
}

bool_t list_is_empty(const list_t* list) { return !list->head; }

void list_push(list_t* list, void* value) {
    list_node_t* node = checked_malloc(sizeof(list_node_t));

    node->prev = list->tail;
    node->next = NULL;
    node->value = value;

    if (list_is_empty(list)) {
        list->head = node;
    } else {
        list->tail->next = node;
    }

    list->tail = node;
}

void* list_pop(list_t* list) {
    list_node_t* node = list->tail;
    void* value;

    if (!node) {
        return NULL;
    }

    value = node->value;

    list->tail = node->prev;
    if (!node->prev) {
        list->head = NULL;
    }

    free(node);
    return value;
}
