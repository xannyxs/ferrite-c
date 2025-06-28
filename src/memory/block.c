#include "memory/block.h"
#include "pmm.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static int32_t mem_num_vpages = 0;

#define NUM_PAGES_DIRS 256

static uint32_t page_dirs[NUM_PAGES_DIRS][1024] __attribute__((aligned(4096)));
static uint8_t page_dirs_used[NUM_PAGES_DIRS];

/* Private */

static void invalidate(uint32_t vaddr) {
  __asm__ __volatile__("invlpg %0" ::"m"(vaddr));
}

static uint32_t *get_current_page_dir(void) {
  uint32_t pd_phys_addr = 0;
  __asm__ __volatile__("movl %%cr3, %0" : "=r"(pd_phys_addr));
  uint32_t pd_virt_addr = pd_phys_addr + KERNEL_VIRTUAL_BASE;

  return (uint32_t *)pd_virt_addr;
}

static void mem_change_page_dir(uint32_t *pd_virt_addr) {
  uint32_t pd_phys_addr = ((uint32_t)pd_virt_addr) - KERNEL_VIRTUAL_BASE;
  __asm__ __volatile__("movl %0, %%cr3" ::"r"(pd_phys_addr));
}

static void sync_page_dirs(void) {
  for (int32_t i = 0; i < NUM_PAGES_DIRS; i++) {
    if (page_dirs_used[i]) {
      uint32_t *page_dir = page_dirs[i];

      for (int32_t i = 768; i < 1023; i++) {
        page_dir[i] = initial_page_dir[i] & ~PAGE_FLAG_OWNER;
      }
    }
  }
}

/* Public */

void mem_map_page(uint32_t vaddr, uint32_t paddr, uint32_t flags) {
  uint32_t *prev_page_dir = 0;

  if (vaddr >= KERNEL_VIRTUAL_BASE) {
    prev_page_dir = get_current_page_dir();
    if (prev_page_dir != initial_page_dir) {
      mem_change_page_dir(initial_page_dir);
    }
  }

  uint32_t pd_index = vaddr >> 22;
  uint32_t pt_index = vaddr >> 12 & 0x3;

  uint32_t *page_dir = REC_PAGEDIR;
  uint32_t *pt = REC_PAGETABLE(pd_index);

  if (!(page_dir[pd_index] & PAGE_FLAG_PRESENT)) {
    uint32_t pt_paddr = pmm_alloc_page_frame();
    page_dir[pd_index] = pt_paddr | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE |
                         PAGE_FLAG_OWNER | flags;
    invalidate(vaddr);

    for (uint32_t i = 0; i < 1024; i++) {
      pt[i] = 0;
    }
  }

  pt[pt_index] = paddr | PAGE_FLAG_PRESENT | flags;
  mem_num_vpages = mem_num_vpages + 1;
  invalidate(vaddr);

  if (prev_page_dir != 0) {
    sync_page_dirs();
    if (prev_page_dir != initial_page_dir) {
      mem_change_page_dir(prev_page_dir);
    }
  }
}

void init_memory(uint32_t mem_upper, uint32_t physical_address_start) {
  initial_page_dir[0] = 0;
  invalidate(0);

  initial_page_dir[1023] = ((uint32_t)initial_page_dir - KERNEL_VIRTUAL_BASE) |
                           PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE;

  invalidate(0xFFFFF000);

  pmm_init(physical_address_start, mem_upper);

  memset(page_dirs, 0, 0x1000 * NUM_PAGES_DIRS);
  memset(page_dirs_used, 0, NUM_PAGES_DIRS);
}
