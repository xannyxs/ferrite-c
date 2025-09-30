#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>

#define HEAP_START (void*)0xD0000000
#define MAXIMUM_SLAB_ALLOCATION 16384
#define MAGIC 0xDEADBEEF

/* Mask of the Flags variable in block_header struct */
#define MEM_ALLOCATOR_MASK 0x01

#define MEM_TYPE_KMALLOC 0x00
#define MEM_TYPE_VMALLOC 0x01

typedef struct free_list {
    size_t size;
    struct free_list* next;
} free_list_t;

typedef struct block_header {
    // Bitfield for memory block properties.
    // Bit 0: Allocator type (0 = kmalloc, 1 = vmalloc)
    // Bits 1-7: Reserved for future use.
    uint8_t flags;
    size_t size;
    uint32_t magic;
} block_header_t;

#endif /* MEMORY_H */
