#include <unistd.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <string.h>
#include "allocator.h"

static allocator_create_func *allocator_create;
static allocator_destroy_func *allocator_destroy;
static allocator_alloc_func *allocator_alloc;
static allocator_free_func *allocator_free;

void print(char* msg) {
    write(STDOUT_FILENO, msg, strlen(msg));
}

Allocator* create_impl(void *const memory, const size_t size) {
    (void) memory;
    (void) size;
    return NULL;
}
void destroy_impl(Allocator *const allocator) {
    (void) allocator;
}
void* alloc_impl(Allocator *const allocator, const size_t size) {
    (void) allocator;
    void* mem = mmap(0, size + sizeof(size_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    if (mem == MAP_FAILED) {
        return NULL;
    }
    size_t* size_ptr = (size_t*) mem;
    *size_ptr = size;
    return mem + sizeof(size_t);
}

void free_impl(Allocator *const allocator, void *const memory) {
    (void) allocator;
    if (memory == NULL) {
        return;
    }
    void* mem = memory - sizeof(size_t);
    munmap(mem, *((size_t*) mem));
}

int main (int argc, char* argv[]) {
    void *library = dlopen(argv[1], RTLD_LOCAL | RTLD_NOW);

    if ((argc > 1) && library) {
        allocator_create = dlsym(library, "allocator_create");
        if (allocator_create == NULL) {
            char* msg = "fail to find create function implementation\n";
            write(STDOUT_FILENO, msg, strlen(msg));
            allocator_create = &create_impl;
        }
        allocator_destroy = dlsym(library, "allocator_destroy");
        if (allocator_create == NULL) {
            char* msg = "fail to find destroy function implementation\n";
            write(STDOUT_FILENO, msg, strlen(msg));
            allocator_destroy = &destroy_impl;
        }
        allocator_alloc = dlsym(library, "allocator_alloc");
        if (allocator_create == NULL) {
            char* msg = "fail to find alloc function implementation\n";
            write(STDOUT_FILENO, msg, strlen(msg));
            allocator_alloc = &alloc_impl;
        }
        allocator_free = dlsym(library, "allocator_free");
        if (allocator_create == NULL) {
            char* msg = "fail to find free function implementation\n";
            write(STDOUT_FILENO, msg, strlen(msg));
            allocator_free = &free_impl;
        }
    } else {
        char* msg = "fail to open library, standard functions is being used\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        allocator_create = &create_impl;
        allocator_destroy = &destroy_impl;
        allocator_alloc = &alloc_impl;
        allocator_free = &free_impl;
    }
    print("start\n");
    void* memory = mmap(0, 1 << 16, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    print("mapped\n");
    Allocator* all = allocator_create(memory, 1 << 16);
    print("allocator created\n");
    char* arr1 = allocator_alloc(all, 20);
    print("arr1 allocated\n");
    char* arr2 = allocator_alloc(all, 10);
    print("arr2 allocated\n");
    allocator_free(all, arr1);
    print("arr1 freed\n");
    arr1 = allocator_alloc(all, 12);
    print("arr1 allocated\n");
    allocator_free(all, arr1);
    allocator_free(all, arr2);
    allocator_destroy(all);
    munmap(memory, 1 << 16);
    if (library) {
        dlclose(library);
    }
    print("all is well\n");
    return 0;
}