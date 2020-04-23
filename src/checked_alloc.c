#include "checked_alloc.h"

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void __attribute__((noreturn)) handle_alloc_error(void) {
    printf("Fatal error: failed to allocate memory: %s", strerror(errno));
    abort();
}

void* checked_malloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) {
        handle_alloc_error();
    }
    return ptr;
}

void* checked_calloc(size_t count, size_t size) {
    void* ptr = calloc(count, size);
    if (!ptr) {
        handle_alloc_error();
    }
    return ptr;
}

void* checked_realloc(void* ptr, size_t new_size) {
    void* new_ptr = realloc(ptr, new_size);
    if (!new_ptr) {
        handle_alloc_error();
    }
    return new_ptr;
}
