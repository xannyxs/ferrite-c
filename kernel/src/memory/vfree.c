#include "arch/x86/memlayout.h"
#include "drivers/printk.h"
#include "lib/stdlib.h"
#include "memory/buddy_allocator/buddy.h"
#include "memory/consts.h"
#include "memory/memory.h"
#include "memory/vmalloc.h"
#include "memory/vmm.h"

void vfree(void* ptr)
{
    if (!ptr) {
        printk("ptr is null\n");
        return;
    }

    block_header_t* header = (block_header_t*)ptr - 1;
    if (header->magic != MAGIC) {
        printk("Ptr is corrupt\n");
        return;
    }

    if ((header->flags & MEM_ALLOCATOR_MASK) != MEM_TYPE_VMALLOC) {
        abort("vfree() is used on a kmalloc() pointer\n");
    }

    u32 block_vaddr = (u32)header;
    size_t block_size = header->size;

    free_list_t* freed_block = (free_list_t*)block_vaddr;
    freed_block->size = block_size;

    free_list_t* current = get_free_list_head();
    free_list_t* prev = NULL;

    while (current && (u32)current < (u32)freed_block) {
        prev = current;
        current = current->next;
    }

    if (prev && (u32)prev + prev->size == (u32)freed_block) {
        prev->size += freed_block->size;
        freed_block = prev;
    } else {
        if (prev) {
            prev->next = freed_block;
        } else {
            set_free_list_head(freed_block);
        }
    }

    if (current && (u32)freed_block + freed_block->size == (u32)current) {
        freed_block->size += current->size;
        freed_block->next = current->next;
    } else {
        freed_block->next = current;
    }

    for (size_t i = 1; i < block_size / PAGE_SIZE; i++) {
        u32 current_vaddr = block_vaddr + (i * PAGE_SIZE);
        void* paddr = vmm_unmap_page((void*)current_vaddr);
        if (paddr) {
            buddy_dealloc((u32)paddr, 0);
        }
    }
}
