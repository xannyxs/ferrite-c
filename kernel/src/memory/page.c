#include "arch/x86/memlayout.h"
#include "drivers/printk.h"
#include "memory/buddy_allocator/buddy.h"
#include "memory/consts.h"

#include <ferrite/string.h>
#include <ferrite/types.h>

/*
 * Allocates a single 4KB page from the buddy allocator, converts it to a
 * virtual address, zeros it, and returns the usable virtual address.
 */
void* get_free_page(void)
{
    void* paddr = buddy_alloc(0);
    if (!paddr) {
        return NULL;
    }

    void* vaddr = (void*)P2V_WO((u32)paddr);
    memset(vaddr, 0, PAGE_SIZE);

    return vaddr;
}

/*
 * Validates and frees a page-aligned virtual address by converting it back to
 * physical and returning it to the buddy allocator.
 */
void free_page(void* ptr)
{
    if (!ptr) {
        printk("ptr is null\n");
        return;
    }
    if ((u32)ptr & (PAGE_SIZE - 1)) {
        printk("free_page: ptr 0x%lx not page-aligned\n", (unsigned long)ptr);
        return;
    }

    u32 paddr = V2P_WO((u32)ptr);
    buddy_dealloc(paddr, 0);
}
