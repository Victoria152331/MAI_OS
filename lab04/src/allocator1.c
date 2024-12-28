#include "allocator.h"
#include <sys/mman.h>

//list

Allocator* allocator_create(void* const memory, const size_t size) {
    Allocator *allocator = (Allocator*) memory;
    size_t *start = (size_t*)(memory + sizeof(Allocator));
    size_t block_size = size - 3 * sizeof(size_t) - sizeof(Allocator);
    *start = block_size << 1; // в начале блока записан его размер
    *(size_t*)((void*)start + block_size) = *start; // в конце блока он продублирован
    *(size_t*)((void*)start  + block_size + sizeof(size_t)) = 0; // после всех блоков находится терминатор
    allocator->ptr = memory + sizeof(Allocator);
    return allocator;
}

void allocator_destroy(Allocator *const allocator) {
    allocator->ptr = NULL;
}

void* allocator_alloc(Allocator *const allocator, const size_t size) {
    size_t* most_suitable = NULL;
    size_t most_suitable_size = 0;
    size_t* current = allocator->ptr;
    size_t cur_size = *current >> 1;
    while(*current) {
        if ((*current) & 1) {
            current = (size_t*)((void*)current + cur_size + 2 * sizeof(size_t));
            cur_size = *current >> 1;
            continue;
        }
        cur_size = *current >> 1;
        if ((cur_size > size) && ((!most_suitable) || (most_suitable_size > cur_size))) {
            most_suitable = current;
            most_suitable_size = cur_size;
        }
        current = (size_t*)((void*)current + cur_size + 2 * sizeof(size_t));
    }
    if (!most_suitable) {
        return NULL;
    }
    if ((long)(most_suitable_size - size - 2 * sizeof(size_t)) > 0) {
        *most_suitable = (size << 1) | 1; //new block start
        *(size_t*)((void*)most_suitable + size + sizeof(size_t)) = (size << 1) | 1; //new block end
        *(size_t*)((void*)most_suitable + size + 2 * sizeof(size_t)) = (most_suitable_size - size - 2 * sizeof(size_t)) << 1; //empty block start
        *(size_t*)((void*)most_suitable + most_suitable_size + sizeof(size_t)) = (most_suitable_size - size - 2 * sizeof(size_t)) << 1; //empty block end
    } else {
        *most_suitable = *most_suitable | 1; // cannot split block
        *(size_t*)((void*)most_suitable + most_suitable_size + sizeof(size_t)) = *most_suitable | 1;
    }
    void* ptr = (void*)most_suitable + sizeof(size_t);
    return ptr;
}

void allocator_free(Allocator *const allocator, void *const memory) {
    size_t* to_free = (size_t*)(memory - sizeof(size_t));
    size_t to_free_size = (*to_free) >> 1;
    *to_free = *to_free & ~1;
    size_t* to_free_end = (size_t*)(memory + to_free_size);
    *to_free_end = *to_free;
    if ((to_free != allocator->ptr) && !(*(to_free - 1) & 1)) { //left is empty
        size_t left_size = *(to_free - 1) >> 1;
        to_free = (size_t*)((void*)to_free - 2 * sizeof(size_t) - left_size);
        *to_free = (to_free_size + left_size + 2 * sizeof(size_t)) << 1;
        *to_free_end = (to_free_size + left_size + 2 * sizeof(size_t)) << 1;
    }
    if ((*(to_free_end + 1) == 0) && !(*(to_free_end + 1) & 1)) { //right is empty
        size_t right_size = *(to_free_end + 1) >> 1;
        to_free_end = (size_t*)((void*)to_free_end + 2 * sizeof(size_t) + right_size);
        *to_free = (to_free_size + right_size + 2 * sizeof(size_t)) << 1;
        *to_free_end = (to_free_size + right_size + 2 * sizeof(size_t)) << 1;
    }
}