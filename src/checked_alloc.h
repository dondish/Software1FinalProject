/**
 * checked_alloc.h - Checked allocation routines that terminate on failure
 * instead of returning NULL.
 */

#ifndef CHECKED_ALLOC_H
#define CHECKED_ALLOC_H

#include <stddef.h>

/**
 * Allocate `size` bytes of memory via `malloc`, terminating the process on
 * failure.
 */
void* checked_malloc(size_t size);

/**
 * Allocate and zero `count * size` bytes via `calloc`, terminating the process
 * on failure.
 */
void* checked_calloc(size_t count, size_t size);

/**
 * Resize the specified memory area to `new_size`, terminating the process on
 * failure.
 */
void* checked_realloc(void* ptr, size_t new_size);

#endif
