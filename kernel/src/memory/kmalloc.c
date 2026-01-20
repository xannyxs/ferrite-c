#include "memory/kmalloc.h"
#include "arch/x86/memlayout.h"
#include "lib/math.h"
#include "memory/buddy_allocator/buddy.h"
#include "memory/consts.h"
#include "memory/memory.h"

#include <types.h>

/* Public */

/**
 * @brief Allocates a physically contiguous memory block.
 *
 * Fast and suitable for most kernel allocations, especially for hardware/DMA
 * buffers that require physical contiguity.
 *
 * @param n The number of bytes to allocate.
 * @return A pointer to the allocated memory, or NULL on failure.
 */
void* kmalloc(size_t n)
{
    if (n == 0 || n >= MAXIMUM_SLAB_ALLOCATION) {
        return NULL;
    }

    size_t total_size = ALIGN(n + sizeof(block_header_t), PAGE_SIZE);

    u32 num_pages = CEIL_DIV(total_size, PAGE_SIZE);
    u32 order = ceil_log2(num_pages);
    void* paddr = buddy_alloc(order);
    if (!paddr) {
        return NULL;
    }

    u32 vaddr = P2V_WO((u32)paddr);
    block_header_t* header = (block_header_t*)vaddr;
    header->magic = MAGIC;
    header->size = total_size;
    header->flags = MEM_TYPE_KMALLOC;

    return (void*)(header + 1);
}
