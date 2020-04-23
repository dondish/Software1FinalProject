/**
 * list.h - Doubly-linked list implementation, used in history and backtrack.
 */

#ifndef LIST_H
#define LIST_H

#include "bool.h"

typedef struct list_node list_node_t;

struct list_node {
    list_node_t* prev;
    list_node_t* next;

    void* value;
};

/**
 * Doubly-linked list.
 */
typedef struct list {
    list_node_t* head;
    list_node_t* tail;
} list_t;

typedef void (*list_item_dtor_t)(void* value);

/**
 * Initialize a new, empty list.
 */
void list_init(list_t* list);

/**
 * Deallocate all items in `list`, invoking `dtor` on each value first.
 */
void list_destroy(list_t* list, list_item_dtor_t dtor);

/**
 * Disconnect and destroy the list tail headed by `node` from `list`, calling
 * the destructor on each item.
 * Note: this is a no-op if `node` is null.
 */
void list_destroy_tail(list_t* list, list_node_t* node, list_item_dtor_t dtor);

/**
 * Check whether `list` is empty.
 */
bool_t list_is_empty(const list_t* list);

/**
 * Append `value` to the end of `list`.
 */
void list_push(list_t* list, void* value);

/**
 * Remove the last node in `list`, returning its value or `NULL` if the list is
 * empty.
 */
void* list_pop(list_t* list);

#endif
