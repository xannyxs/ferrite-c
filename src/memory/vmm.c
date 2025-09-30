#include "memory/vmm.h"
#include "arch/x86/memlayout.h"
#include "drivers/printk.h"
#include "lib/stdlib.h"
#include "memory/buddy_allocator/buddy.h"
#include "memory/consts.h"
#include "memory/memblock.h"
#include "memory/pmm.h"
#include "page.h"
#include "string.h"

#include <stdint.h>

/* i386 does not support invld. Using flush_tlb() instead
 * https://wiki.osdev.org/TLB
 */
extern void flush_tlb(void);
extern void load_page_directory(uint32_t*);
extern void enable_paging(void);

uint32_t page_directory[1024] __attribute__((aligned(4096)));

void visualize_paging(uint32_t limit_mb, uint32_t detailed_mb)
{
    printk("--- Paging Visualization ---\n");
    printk("Scanning up to %u MB of virtual address space.\n", limit_mb);
    printk("'.' = Mapped Page (4KB) | ' ' = Unmapped Page | 'X' = Unmapped "
           "Region (4MB)\n\n");

    uint32_t* page_directory = (uintptr_t*)0xFFFFF000;
    uint32_t pd_limit = limit_mb / 4;

    for (uint32_t pd_index = 0; pd_index < pd_limit; ++pd_index) {
        uintptr_t base_vaddr = pd_index * 0x400000; // 4 MB chunks

        if (page_directory[pd_index] & PTE_P) {
            printk("VAddr 0x%x - 0x%x: [", base_vaddr, base_vaddr + 0x3FFFFF);

            if ((pd_index * 4) < detailed_mb) {
                uint32_t* page_table = &((uintptr_t*)0xFFC00000)[pd_index * 1024];

                for (int pt_index = 0; pt_index < 1024; ++pt_index) {
                    if (page_table[pt_index] & PTE_P) {
                        printk(".");
                    } else {
                        printk(" ");
                    }
                }
            } else {
                printk("Mapped Region (Details omitted)");
            }
            printk("]\n");

        } else {
            printk("VAddr 0x%x - 0x%x: [X]\n", base_vaddr, base_vaddr + 0x3FFFFF);
        }

        if ((pd_index + 1) * 4 == detailed_mb) {
            printk("\n--- End of Detailed View ---\n\n");
        }
    }
    printk("--- End of Visualization ---\n");
}

/* Public */

void vmm_free_pagedir(void* pgdir)
{
    uint32_t* pgdir_addr = (uint32_t*)pgdir;

    for (int32_t i = 0; i < 768; i += 1) {
        if (pgdir_addr[i] & PTE_P && pgdir_addr[i] != page_directory[i]) {
            uint32_t pt_paddr = pgdir_addr[i] & ~0xFFF;
            free_page((void*)P2V_WO(pt_paddr));
        }
    }

    free_page(pgdir_addr);
}

void* vmm_unmap_page(void* vaddr)
{
    uint32_t pdindex = (uint32_t)vaddr >> 22;
    uint32_t ptindex = (uint32_t)vaddr >> 12 & 0x03FF;

    uint32_t* pt = (uint32_t*)(0xFFC00000 + (pdindex * PAGE_SIZE));
    uint32_t* pte = &pt[ptindex];

    if (!(*pte & PTE_P)) {
        return NULL;
    }

    void* paddr = (void*)(*pte & ~0xFFF);
    *pte = 0;
    flush_tlb();

    return paddr;
}

/**
 * FIXME: Need to create a logger instead of using printk
 * FIXME: Create wrapper for memblock / buddy Alloc
 *
 * @brief Maps a physical address to a virtual address.
 *
 * @return 0 on success.
 * @return -1 if the mapping already exists.
 */
__attribute__((warn_unused_result)) int32_t vmm_map_page(void* physaddr,
    void* virtualaddr,
    uint32_t flags)
{
    uint32_t pdindex = (uint32_t)virtualaddr >> 22;
    uint32_t ptindex = (uint32_t)virtualaddr >> 12 & 0x03FF;

    uint32_t* pd = (uint32_t*)0xFFFFF000;
    uint32_t* pt = (uint32_t*)(0xFFC00000 + (pdindex * PAGE_SIZE));

    if (!(pd[pdindex] & PTE_P)) {
        uintptr_t paddr = 0;
        if (memblock_is_active() == true) {
            paddr = (uintptr_t)memblock(PAGE_SIZE);
        } else {
            paddr = (uintptr_t)buddy_alloc(0);
        }

        if (paddr == 0) {
            abort("Out of physical memory");
        }

        pd[pdindex] = paddr | PTE_U | PTE_W | PTE_P;
        flush_tlb();

        memset(pt, 0, PAGE_SIZE);
    }

    if (pt[ptindex] & PTE_P) {
        printk("Mapping already exists\n");
        return -1;
    }

    pt[ptindex] = ((uint32_t)physaddr) | (flags & 0xFFF) | PTE_P;

    flush_tlb();
    return 0;
}

void vmm_remap_page(void* vaddr, void* paddr, int32_t flags)
{
    uint32_t pdindex = (uint32_t)vaddr >> 22;
    uint32_t ptindex = (uint32_t)vaddr >> 12 & 0x03FF;

    uint32_t* pt = (uint32_t*)(0xFFC00000 + (pdindex * PAGE_SIZE));
    pt[ptindex] = ((uint32_t)paddr) | (flags & 0xFFF) | PTE_P;

    flush_tlb();
}

void vmm_init_pages(void)
{
    memset(page_directory, 0, sizeof(uint32_t) * 1024);

    size_t memory_to_map = ZONE_NORMAL;
    size_t total_ram = (pmm_bitmap_len() * PAGE_SIZE * 8);
    if (ZONE_NORMAL > total_ram) {
        memory_to_map = total_ram;
    }

    for (uint32_t paddr = 0; paddr < memory_to_map; paddr += PAGE_SIZE * 1024) {
        void* pt_paddr = memblock(PAGE_SIZE);
        if (!pt_paddr) {
            abort("Out of memory allocating a page table");
        }

        uint32_t* pt_vaddr = (uint32_t*)P2V_WO((uintptr_t)pt_paddr);
        for (int i = 0; i < 1024; i++) {
            uint32_t page_phys_addr = paddr + (i * PAGE_SIZE);
            if (page_phys_addr < memory_to_map) {
                pt_vaddr[i] = page_phys_addr | PTE_P | PTE_W | PTE_U;
                continue;
            }

            pt_vaddr[i] = 0;
        }

        uint32_t pd_index = (KERNBASE + paddr) / 0x400000;
        page_directory[pd_index] = (uintptr_t)pt_paddr | PTE_P | PTE_W | PTE_U;

        uint32_t identity_pd_index = paddr / 0x400000;
        page_directory[identity_pd_index] = (uintptr_t)pt_paddr | PTE_P | PTE_W | PTE_U;
    }

    uint32_t page_directory_paddr = V2P_WO((uint32_t)page_directory);
    page_directory[1023] = (uintptr_t)page_directory_paddr | PTE_P | PTE_W | PTE_U;

    load_page_directory((uint32_t*)page_directory_paddr);
    enable_paging();

    /* Creating a scratch map */
    void* paddr = memblock(PAGE_SIZE);
    if (!paddr) {
        abort("Something went wrong");
    }

    int32_t ret = vmm_map_page(paddr, SCRATCH_VADDR, PTE_W | PTE_U);
    if (ret < 0) {
        abort("Scratch Page is already taken\n");
    }

    // visualize_paging(8, 8);
}
