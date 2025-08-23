#include "memory/vmm.h"
#include "arch/x86/memlayout.h"
#include "debug/debug.h"
#include "drivers/printk.h"
#include "lib/stdlib.h"
#include "memory/buddy_allocator/buddy.h"
#include "memory/consts.h"
#include "memory/memblock.h"
#include "memory/pmm.h"
#include "string.h"

#include <stdint.h>

/* i386 does not support invld. Using flush_tlb() instead
 * https://wiki.osdev.org/TLB
 */
extern void flush_tlb(void);
extern void load_page_directory(uint32_t *);
extern void enable_paging(void);

static uint32_t page_directory[1024] __attribute__((aligned(4096)));

void visualize_paging(uint32_t limit_mb, uint32_t detailed_mb) {
  printk("--- Paging Visualization ---\n");
  printk("Scanning up to %u MB of virtual address space.\n", limit_mb);
  printk("'.' = Mapped Page (4KB) | ' ' = Unmapped Page | 'X' = Unmapped "
         "Region (4MB)\n\n");

  uint32_t *page_directory = (uintptr_t *)0xFFFFF000;
  uint32_t pd_limit = limit_mb / 4;

  for (uint32_t pd_index = 0; pd_index < pd_limit; ++pd_index) {
    uintptr_t base_vaddr = pd_index * 0x400000; // 4 MB chunks

    if (page_directory[pd_index] & PAGE_FLAG_PRESENT) {
      printk("VAddr 0x%x - 0x%x: [", base_vaddr, base_vaddr + 0x3FFFFF);

      if ((pd_index * 4) < detailed_mb) {
        uint32_t *page_table = &((uintptr_t *)0xFFC00000)[pd_index * 1024];

        for (int pt_index = 0; pt_index < 1024; ++pt_index) {
          if (page_table[pt_index] & PAGE_FLAG_PRESENT) {
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

void *vmm_unmap_page(void *vaddr) {
  uint32_t pdindex = (uint32_t)vaddr >> 22;
  uint32_t ptindex = (uint32_t)vaddr >> 12 & 0x03FF;

  uint32_t *pt = (uint32_t *)(0xFFC00000 + (pdindex * PAGE_SIZE));
  uint32_t *pte = &pt[ptindex];

  if (!(*pte & PAGE_FLAG_PRESENT)) {
    return NULL;
  }

  void *paddr = (void *)(*pte & ~0xFFF);
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
__attribute__((warn_unused_result)) int32_t vmm_map_page(void *physaddr,
                                                         void *virtualaddr,
                                                         uint32_t flags) {
  uint32_t pdindex = (uint32_t)virtualaddr >> 22;
  uint32_t ptindex = (uint32_t)virtualaddr >> 12 & 0x03FF;

  uint32_t *pd = (uint32_t *)0xFFFFF000;
  uint32_t *pt = (uint32_t *)(0xFFC00000 + (pdindex * PAGE_SIZE));

  if (!(pd[pdindex] & PAGE_FLAG_PRESENT)) {
    uintptr_t paddr = 0;
    if (memblock_is_active() == true) {
      paddr = (uintptr_t)memblock(PAGE_SIZE);
    } else {
      paddr = (uintptr_t)buddy_alloc(0);
    }

    if (paddr == 0) {
      abort("Out of physical memory");
    }

    pd[pdindex] =
        paddr | PAGE_FLAG_SUPERVISOR | PAGE_FLAG_WRITABLE | PAGE_FLAG_PRESENT;
    flush_tlb();

    memset(pt, 0, PAGE_SIZE);
  }

  if (pt[ptindex] & PAGE_FLAG_PRESENT) {
    printk("Mapping already exists\n");
    return -1;
  }

  pt[ptindex] = ((uint32_t)physaddr) | (flags & 0xFFF) | PAGE_FLAG_PRESENT;

  flush_tlb();
  return 0;
}

void vmm_remap_page(void *vaddr, void *paddr, int32_t flags) {
  uint32_t pdindex = (uint32_t)vaddr >> 22;
  uint32_t ptindex = (uint32_t)vaddr >> 12 & 0x03FF;

  uint32_t *pt = (uint32_t *)(0xFFC00000 + (pdindex * PAGE_SIZE));
  pt[ptindex] = ((uint32_t)paddr) | (flags & 0xFFF) | PAGE_FLAG_PRESENT;

  flush_tlb();
}

void vmm_init_pages(void) {
  memset(page_directory, 0, sizeof(uint32_t) * 1024);

  size_t memory_to_map = ZONE_NORMAL;
  size_t total_ram = (pmm_bitmap_len() * PAGE_SIZE * 8) - pmm_get_first_addr();
  if (ZONE_NORMAL > total_ram) {
    memory_to_map = total_ram;
  }

  for (uint32_t paddr = 0; paddr < memory_to_map; paddr += PAGE_SIZE * 1024) {
    void *pt_paddr = memblock(PAGE_SIZE);
    if (!pt_paddr) {
      abort("Out of memory allocating a page table");
    }

    uint32_t *pt_vaddr = (uint32_t *)P2V_WO((uintptr_t)pt_paddr);
    for (int i = 0; i < 1024; i++) {
      uint32_t page_phys_addr = paddr + (i * PAGE_SIZE);
      if (page_phys_addr < memory_to_map) {
        pt_vaddr[i] = page_phys_addr | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITABLE |
                      PAGE_FLAG_SUPERVISOR;
        continue;
      }

      pt_vaddr[i] = 0;
    }

    uint32_t pd_index = (KERNBASE + paddr) / 0x400000;
    page_directory[pd_index] = (uintptr_t)pt_paddr | PAGE_FLAG_PRESENT |
                               PAGE_FLAG_WRITABLE | PAGE_FLAG_SUPERVISOR;

    uint32_t identity_pd_index = paddr / 0x400000;
    page_directory[identity_pd_index] = (uintptr_t)pt_paddr |
                                        PAGE_FLAG_PRESENT | PAGE_FLAG_WRITABLE |
                                        PAGE_FLAG_SUPERVISOR;
  }

  uint32_t page_directory_paddr = V2P_WO((uint32_t)page_directory);
  page_directory[1023] = (uintptr_t)page_directory_paddr | PAGE_FLAG_PRESENT |
                         PAGE_FLAG_WRITABLE | PAGE_FLAG_SUPERVISOR;

  load_page_directory((uint32_t *)page_directory_paddr);
  enable_paging();

  /* Creating a scratch map */
  void *paddr = memblock(PAGE_SIZE);
  if (!paddr) {
    abort("Something went wrong");
  }

  int32_t ret = vmm_map_page(paddr, SCRATCH_VADDR,
                             PAGE_FLAG_WRITABLE | PAGE_FLAG_SUPERVISOR);
  if (ret < 0) {
    abort("Scratch Page is already taken\n");
  }

  visualize_paging(32, 32);
}
