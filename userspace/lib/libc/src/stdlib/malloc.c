#include <libc/stdlib.h>
#include <libc/string.h>
#include <libc/syscalls.h>

typedef struct block {
    size_t size;
    struct block* next;
    int free;
} block_t;

#define BLOCK_SIZE sizeof(block_t)
#define ALIGN4(x) (((x) + 3) & ~3)

static block_t* heap_start = NULL;

static block_t* find_free_block(size_t size)
{
    block_t* current = heap_start;

    while (current) {
        if (current->free && current->size >= size)
            return current;

        current = (block_t*)((char*)current + BLOCK_SIZE + current->size);

        if ((void*)current >= sbrk(0))
            break;
    }

    return NULL;
}

static block_t* request_space(size_t size)
{
    block_t* block;
    void* request = sbrk(BLOCK_SIZE + size);

    if (request == (void*)-1)
        return NULL;

    block = (block_t*)request;
    block->size = size;
    block->next = NULL;
    block->free = 0;

    return block;
}

void* malloc(size_t size)
{
    if (size == 0)
        return NULL;

    size = ALIGN4(size);

    block_t* block;

    if (!heap_start) {
        block = request_space(size);
        if (!block)
            return NULL;
        heap_start = block;
    } else {
        block = find_free_block(size);

        if (block) {
            block->free = 0;
        } else {
            block = request_space(size);
            if (!block)
                return NULL;
        }
    }

    return (void*)(block + 1);
}

static block_t* get_block(void* ptr) { return (block_t*)ptr - 1; }

void free(void* ptr)
{
    if (!ptr)
        return;

    block_t* block = get_block(ptr);
    block->free = 1;

    block_t* next = (block_t*)((char*)block + BLOCK_SIZE + block->size);

    if ((void*)next < sbrk(0) && next->free) {
        block->size += BLOCK_SIZE + next->size;
    }
}
