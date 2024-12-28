#include "allocator.h"
#include <sys/mman.h>

//buddy

Allocator* allocator_create(void* const memory, const size_t size) {
    Allocator *allocator = (Allocator*) memory;
    char *start = (char*)(memory + sizeof(Allocator));
    size_t block_size = size - 1 - sizeof(Allocator);
    char block_rang = 0;
    while (block_size > 1) {
        block_rang++;
        block_size >>= 1;
    }
    *start = block_rang << 1; // в начале блока записан его размер
    *(size_t*)((void*)start + (1 << block_rang) - 1) = *start; // в конце блока он продублирован
    *(size_t*)((void*)start + (1 << block_rang)) = -1; // после всех блоков находится терминатор
    allocator->ptr = memory + sizeof(Allocator);
    return allocator;
}

void allocator_destroy(Allocator *const allocator) {
    allocator->ptr = NULL;
}

void* allocator_alloc(Allocator *const allocator, const size_t size) {
    char* most_suitable = NULL;
    char most_suitable_rang = 0;
    char* current = allocator->ptr;
    char cur_rang = *current >> 1;
    char need_size = size + 2;
    char need_rang = 0;
    while (need_size > 1) {
        need_rang++;
        need_size >>= 1;
    }
    while(*current != -1) {
        if ((*current) & 1) {
            current = (current + (1 << cur_rang));
            cur_rang= *current >> 1;
            continue;
        }
        cur_rang = *current >> 1;
        if ((cur_rang >= need_rang) && ((!most_suitable) || (most_suitable_rang > cur_rang))) {
            most_suitable = current;
            most_suitable_rang = cur_rang;
        }
        current = (current + (1 << cur_rang));
    }
    if (!most_suitable) {
        return NULL;
    }
    
    cur_rang = most_suitable_rang;

    while (cur_rang != need_rang) {
        cur_rang --;
        *most_suitable = cur_rang << 1;
        *(most_suitable + (1 << cur_rang) - 1) = cur_rang << 1;
        *(most_suitable + (1 << cur_rang)) = cur_rang << 1;
        *(most_suitable + (1 << (cur_rang + 1)) - 1) = cur_rang << 1;
    }
    *most_suitable = *most_suitable | 1;
    *(most_suitable + (1 << need_rang) - 1) = *most_suitable | 1;

    void* ptr = (void*)most_suitable + 1;
    return ptr;
}

void allocator_free(Allocator *const allocator, void *const memory) {
    char* to_free = memory - 1;
    char to_free_rang = (*to_free) >> 1;
    *to_free = *to_free & ~1;
    char* to_free_end = (memory + (1 << to_free_rang) - 1);
    *to_free_end = *to_free;
    size_t location = 0;
    char* temp = allocator->ptr;
    while (temp != to_free) {
        temp = temp + 1;
        location++;
    }
    location >>= 2;
    char cur_rang = to_free_rang;
    while(1) {
        if (!*(to_free + (1 << cur_rang)) && !(location & 1)) { //terminator and block is left
            break;
        }
        if (location & 1) { //block is rigth
            if (!(*(to_free - 1) & 1)) { //left block is not empty
                break;
            }
            *(to_free - (1 << cur_rang)) = (cur_rang + 1) << 1;
            *(to_free + (1 << cur_rang) - 1) = (cur_rang + 1) << 1;
            cur_rang ++;
        } else { //block is left
            if (!(*(to_free + (1 << cur_rang)) & 1)) { //right block is not empty
                break;
            }
            *to_free = (cur_rang + 1) << 1;
            *(to_free + (1 << (cur_rang + 1)) - 1) = (cur_rang + 1) << 1;
            cur_rang ++;
        }
        location >>= 1;
    }
}