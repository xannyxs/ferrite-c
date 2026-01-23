#include "memory/vmm.h"
#include "arch/x86/memlayout.h"
#include "drivers/printk.h"
#include "ferrite/string.h"
#include "ferrite/types.h"
#include "lib/math.h"
#include "lib/stdlib.h"
#include "memory/buddy_allocator/buddy.h"
#include "memory/consts.h"
#include "memory/memblock.h"
#include "memory/pmm.h"
#include "page.h"

/* i386 does not support invld. Using flush_tlb() instead
 * https://wiki.osdev.org/TLB
 */
extern void flush_tlb(void);
extern void load_page_directory(u32*);
extern void enable_paging(void);

u32 page_directory[1024] __attribute__((aligned(4096)));

void visualize_paging(u32 limit_mb, u32 detailed_mb)
{
    printk("--- Paging Visualization ---\n");
    printk("Scanning up to %u MB of virtual address space.\n", limit_mb);
    printk("'.' = Mapped Page (4KB) | ' ' = Unmapped Page | 'X' = Unmapped "
           "Region (4MB)\n\n");

    u32* page_directory = (u32*)0xFFFFF000;
    u32 pd_limit = limit_mb / 4;

    for (u32 pd_index = 0; pd_index < pd_limit; ++pd_index) {
        u32 base_vaddr = pd_index * 0x400000; // 4 MB chunks

        if (page_directory[pd_index] & PTE_P) {
            printk("VAddr 0x%x - 0x%x: [", base_vaddr, base_vaddr + 0x3FFFFF);

            if ((pd_index * 4) < detailed_mb) {
                u32* page_table = &((u32*)0xFFC00000)[pd_index * 1024];

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
            printk(
                "VAddr 0x%x - 0x%x: [X]\n", base_vaddr, base_vaddr + 0x3FFFFF
            );
        }

        if ((pd_index + 1) * 4 == detailed_mb) {
            printk("\n--- End of Detailed View ---\n\n");
        }
    }
    printk("--- End of Visualization ---\n");
}

/* Public */

void vmm_clear_pages(void)
{
    u32* pd = (u32*)0xFFFFF000;

    for (u32 pdi = 0; pdi < 768; pdi++) {
        if (!(pd[pdi] & PTE_P)) {
            continue;
        }

        u32* pt = (u32*)(0xFFC00000 + (pdi * PAGE_SIZE));
        for (u32 pti = 0; pti < 1024; pti++) {
            if (pt[pti] & PTE_P) {
                u32 paddr = pt[pti] & PAGE_MASK;

                if (buddy_manages((paddr_t)paddr)) {
                    buddy_dealloc((paddr_t)paddr, 0);
                }
                pt[pti] = 0;
            }
        }

        u32 pt_phys = pd[pdi] & PAGE_MASK;
        pd[pdi] = 0;

        if (buddy_manages((paddr_t)pt_phys)) {
            buddy_dealloc((paddr_t)pt_phys, 0);
        }
    }

    flush_tlb();
}

void vmm_free_pagedir(void* pgdir)
{
    u32* pgdir_addr = (u32*)pgdir;

    for (s32 i = 0; i < 768; i += 1) {
        if (pgdir_addr[i] & PTE_P && pgdir_addr[i] != page_directory[i]) {
            u32 pt_paddr = pgdir_addr[i] & ~0xFFF;
            u32* pt_vaddr = (u32*)P2V_WO(pt_paddr);

            for (s32 j = 0; j < 1024; j += 1) {
                if (pt_vaddr[j] & PTE_P) {
                    u32 page_paddr = pt_vaddr[j] & ~0xFFF;
                    free_page((void*)P2V_WO(page_paddr));
                }
            }

            free_page(pt_vaddr);
        }
    }
    free_page(pgdir_addr);
}

void* vmm_unmap_page(void* vaddr)
{
    u32* pd = (u32*)0xFFFFF000;
    u32 pdindex = (u32)vaddr >> 22;
    u32 ptindex = (u32)vaddr >> 12 & 0x03FF;

    if (!(pd[pdindex] & PTE_P)) {
        return NULL;
    }

    u32* pt = (u32*)(0xFFC00000 + (pdindex * PAGE_SIZE));
    u32* pte = &pt[ptindex];

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
__attribute__((warn_unused_result)) s32
vmm_map_page(void* paddr, void* vaddr, u32 flags)
{
    if (!paddr) {
        paddr = buddy_alloc(0);
        if (!paddr) {
            return -1;
        }
    }

    u32 pdindex = (u32)vaddr >> 22;
    u32 ptindex = (u32)vaddr >> 12 & 0x03FF;

    u32* pd = (u32*)0xFFFFF000;
    u32* pt = (u32*)(0xFFC00000 + (pdindex * PAGE_SIZE));

    if (!(pd[pdindex] & PTE_P)) {
        u32 pt_paddr = 0;

        if (memblock_is_active() == true) {
            pt_paddr = (u32)memblock(PAGE_SIZE);
        } else {
            pt_paddr = (u32)buddy_alloc(0);
        }

        if (!pt_paddr) {
            abort("Out of physical memory");
        }

        pd[pdindex] = pt_paddr | PTE_U | PTE_W | PTE_P;
        flush_tlb();

        memset(pt, 0, PAGE_SIZE);
    }

    if (pt[ptindex] & PTE_P) {
        return -1;
    }

    pt[ptindex] = ((u32)paddr) | (flags & 0xFFF) | PTE_P;

    flush_tlb();
    return 0;
}

void vmm_remap_page(void* vaddr, void* paddr, s32 flags)
{
    u32 pdindex = (u32)vaddr >> 22;
    u32 ptindex = (u32)vaddr >> 12 & 0x03FF;

    u32* pt = (u32*)(0xFFC00000 + (pdindex * PAGE_SIZE));
    pt[ptindex] = ((u32)paddr) | (flags & 0xFFF) | PTE_P;

    flush_tlb();
}

void vmm_init_pages(void)
{
    memset(page_directory, 0, sizeof(u32) * 1024);

    size_t memory_to_map = ZONE_NORMAL;
    size_t total_ram = (pmm_bitmap_len() * PAGE_SIZE * 8);
    if (ZONE_NORMAL > total_ram) {
        memory_to_map = total_ram;
    }

    for (u32 paddr = 0; paddr < memory_to_map; paddr += PAGE_SIZE * 1024) {
        void* pt_paddr = memblock(PAGE_SIZE);
        if (!pt_paddr) {
            abort("Out of memory allocating a page table");
        }

        u32* pt_vaddr = (u32*)P2V_WO((u32)pt_paddr);
        for (int i = 0; i < 1024; i++) {
            u32 page_phys_addr = paddr + (i * PAGE_SIZE);
            if (page_phys_addr < memory_to_map) {
                pt_vaddr[i] = page_phys_addr | PTE_P | PTE_W | PTE_U;
                continue;
            }

            pt_vaddr[i] = 0;
        }

        u32 pd_index = (KERNBASE + paddr) / 0x400000;
        page_directory[pd_index] = (u32)pt_paddr | PTE_P | PTE_W | PTE_U;

        u32 identity_pd_index = paddr / 0x400000;
        page_directory[identity_pd_index]
            = (u32)pt_paddr | PTE_P | PTE_W | PTE_U;
    }

    u32 page_directory_paddr = V2P_WO((u32)page_directory);
    page_directory[1023] = page_directory_paddr | PTE_P | PTE_W;

    load_page_directory((u32*)page_directory_paddr);
    enable_paging();

    /* Creating a scratch map */
    void* paddr = memblock(PAGE_SIZE);
    if (!paddr) {
        abort("Something went wrong");
    }

    s32 ret = vmm_map_page(paddr, SCRATCH_VADDR, PTE_W | PTE_U);
    if (ret < 0) {
        abort("Scratch Page is already taken\n");
    }
}
