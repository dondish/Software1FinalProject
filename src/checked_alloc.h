#ifndef CHECKED_ALLOC_H
#define CHECKED_ALLOC_H

#include <stddef.h>

void* checked_malloc(size_t size);
void* checked_calloc(size_t count, size_t size);

#endif
