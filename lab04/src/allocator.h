#ifndef ALLOCATOR
#define ALLOCATOR

#include <stddef.h>

typedef struct {
    void *ptr;
} Allocator;

typedef Allocator* allocator_create_func(void *const memory, const size_t size);
typedef void allocator_destroy_func(Allocator *const allocator);
typedef void* allocator_alloc_func(Allocator *const allocator, const size_t size);
typedef void allocator_free_func(Allocator *const allocator, void *const memory);

#endif