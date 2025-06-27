#include "memory/block.h"
#include "arch/x86/multiboot.h"
#include "drivers/printk.h"
#include "lib/math.h"
#include "lib/stdlib.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static uint32_t page_frame_min;
static uint32_t page_frame_max;
static uint32_t total_alloc;
static int32_t mem_num_vpages = 0;

#define NUM_PAGES_DIRS 256
#define NUM_PAGE_FRAMES (0x2000000 / 0x1000)

uint8_t pmm_bitmap[NUM_PAGE_FRAMES / 8]; // Dynamic, bit array

static uint32_t page_dirs[NUM_PAGES_DIRS][1024] __attribute__((aligned(4096)));
static uint8_t page_dirs_used[NUM_PAGES_DIRS];

/* Private */

static void pmm_print_bitmap() {
  printk("--- PMM Bitmap State ---\n");
  for (int i = 0; i < 256; i++) {
    uint8_t byte = pmm_bitmap[i];
    for (int j = 7; j >= 0; j--) {
      if ((byte >> j) & 1) {
        printk("1");
      } else {
        printk("0");
      }
    }
    printk(" ");
    if ((i + 1) % 16 == 0) {
      printk("\n");
    }
  }
  printk("------------------------\n");
}

static inline void pmm_clear_bit(uint32_t paddr) {
  uint32_t i = paddr / 0x1000;
  uint32_t byte = i / 8;
  uint8_t bit = i % 8;

  pmm_bitmap[byte] &= ~(1 << bit);
}

static inline void pmm_set_bit(uint32_t paddr) {
  uint32_t i = paddr / 0x1000;
  uint32_t byte = i / 8;
  uint8_t bit = i % 8;

  pmm_bitmap[byte] |= (1 << bit);
}

void pmm_init_from_map(multiboot_info_t *mbd) {
  uint32_t kernel_physical_end = (uint32_t)&_physical_kernel_end;
  printk("Kernel physical end: 0x%x\n", kernel_physical_end);

  memset(pmm_bitmap, 0xFF, sizeof(pmm_bitmap));

  for (uint32_t i = 0; i < mbd->mmap_length;
       i += sizeof(multiboot_memory_map_t)) {
    multiboot_memory_map_t *mmmt =
        (multiboot_memory_map_t *)(mbd->mmap_addr + i);

    if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE) {
      printk("Start Addr Low: 0x%x | Length Low: 0x%x\n", mmmt->addr_low,
             mmmt->len_low);

      for (uint32_t j = 0; j < mmmt->len_low; j += 0x1000) {
        uint32_t current_page_addr = mmmt->addr_low + j;

        if (current_page_addr >= kernel_physical_end) {
          pmm_clear_bit(current_page_addr);
        }
      }
    }
  }

  pmm_print_bitmap();
  printk("PMM bitmap initialized.\n");
}

static void invalidate(uint32_t vaddr) {
  __asm__ __volatile__("invlpg %0" ::"m"(vaddr));
}

static void pmm_init(uint32_t mem_low, uint32_t mem_upper) {
  page_frame_min = CEIL_DIV(mem_low, 0x1000);
  page_frame_max = mem_upper / 0x1000;
  total_alloc = 0;

  memset(pmm_bitmap, 0, sizeof(pmm_bitmap));
}

uint32_t pmm_alloc_page_frame(void) {
  uint32_t start = page_frame_min / 8 + ((page_frame_min & 7) != 0 ? 1 : 0);
  uint32_t end = page_frame_max / 8 - ((page_frame_max & 7) != 0 ? 1 : 0);

  for (uint32_t b = start; b < end; b++) {
    uint8_t byte = pmm_bitmap[b];
    if (byte == 0xFF) {
      continue;
    }

    for (uint32_t i = 0; i < 8; i++) {
      bool used = byte >> i & 1;

      if (!used) {
        byte ^= (-1 ^ byte) & (1 << i);
        pmm_bitmap[b] = byte;
        total_alloc = total_alloc + 1;

        uint32_t paddr = (b * 8 + i) * 0x1000;

        return paddr;
      }
    }
  }

  // TODO: Give proper error handling
  return 0;
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
