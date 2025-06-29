#include "memory/block.h"
#include "debug/debug.h"
#include "lib/stdlib.h"
#include "pmm.h"

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

/* static uint32_t *get_current_page_dir(void) {
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
      for (int32_t j = 768; j < 1023; j++) {
        page_dir[j] = initial_page_dir[j] & ~PAGE_FLAG_OWNER;
      }
    }
  }
} */

/* Public */

void mem_map_page(uint32_t vaddr, uint32_t paddr, uint32_t flags) {
  uint32_t pd_index = vaddr >> 22;
  uint32_t pt_index = (vaddr >> 12) & 0x3FF;

  uint32_t *page_dir = REC_PAGEDIR;
  uint32_t *pt = REC_PAGETABLE(pd_index);

  bochs_print_string("hello 1\n");
  BOCHS_MAGICBREAK();

  if (!(page_dir[pd_index] & PAGE_FLAG_PRESENT)) {
    uint32_t pt_paddr = pmm_alloc_page();
    if (pt_paddr == 0) {
      abort("FATAL: Out of memory while creating new page table.");
    }

    bochs_print_string("hello 2\n");
    BOCHS_MAGICBREAK();

    page_dir[pd_index] = (pt_paddr & 0xFFFFF000) | PAGE_FLAG_PRESENT |
                         PAGE_FLAG_WRITE | PAGE_FLAG_OWNER | flags;
    invalidate((uint32_t)pt);

    bochs_print_string("hello 3\n");
    BOCHS_MAGICBREAK();

    memset(pt, 0, PAGE_SIZE);
  }

  bochs_print_string("hello 4\n");
  BOCHS_MAGICBREAK();

  pt[pt_index] = (paddr & 0xFFFFF000) | PAGE_FLAG_PRESENT | flags;
  mem_num_vpages = mem_num_vpages + 1;
  invalidate(vaddr);
}

void init_memory(void) {
  initial_page_dir[0] = 0;
  invalidate(0);

  uint32_t pd_phys_addr = (uint32_t)initial_page_dir - KERNEL_VIRTUAL_BASE;
  initial_page_dir[1023] =
      (pd_phys_addr & 0xFFFFF000) | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE;

  invalidate(0xFFFFF000);

  memset(page_dirs, 0, PAGE_SIZE * NUM_PAGES_DIRS);
  memset(page_dirs_used, 0, NUM_PAGES_DIRS);
}
