#include "memory/memblock.h"
#include "memory/consts.h"
#include "memory/pmm.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static bool bumpalloc_is_active = false;
static void* next_free_addr = NULL;
static void* heap_end_addr = NULL;

static void kbrk(size_t heap_size)
{
    next_free_addr = (void*)pmm_get_first_addr();
    heap_end_addr = (void*)heap_size;
}

/* Public */

void* get_next_free_addr(void) { return next_free_addr; }

void* get_heap_end_addr(void) { return heap_end_addr; }

void memblock_deactivate(void) { bumpalloc_is_active = false; }

bool memblock_is_active(void) { return bumpalloc_is_active; }

/*
 * FIXME: This implementation only uses the first available memory region and
 * assumes it is contiguous. It does not correctly handle fragmented
 * physical memory maps.
 *
 * Future work should involve parsing the entire memory map to manage
 * multiple, non-contiguous free regions.
 *
 * Example:
 * map[0000001000] 1 being taken, 0 being free. Will be devestating for our
 * memblock()
 */
void memblock_init(void)
{
    kbrk(pmm_bitmap_len() * PAGE_SIZE * 8);

    bumpalloc_is_active = true;
}

/*
 * WARNING:
 * Memblock is meant for early allocation & returns a Physical Address.
 * Please use with cautiously.
 */
void* memblock(size_t num_bytes)
{
    if (num_bytes == 0) {
        return NULL;
    }

    void* new_heap_end = (void*)((uintptr_t)next_free_addr + num_bytes);
    if (new_heap_end > heap_end_addr) {
        return NULL;
    }

    void* ptr = next_free_addr;
    next_free_addr = (void*)((uintptr_t)next_free_addr + num_bytes);

    return ptr;
}
