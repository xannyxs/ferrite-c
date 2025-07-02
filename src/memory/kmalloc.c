#include "memory/kmalloc.h"
#include "lib/math.h"
#include "lib/stdlib.h"
#include "memory/pmm.h"
#include "memory/vmm.h"

#include <stddef.h>
#include <stdint.h>

void *kmalloc(size_t num_bytes) {
  if (num_bytes == 0) {
    return NULL;
  }

  uint32_t pages_needed = CEIL_DIV(num_bytes, PAGE_SIZE);
  void *vaddr = vmm_find_free_region(pages_needed);
  if (vaddr == NULL) {
    // No more virtual address space!
    return NULL;
  }

  for (uint32_t i = 0; i < pages_needed; i++) {
    void *ptr = (void *)pmm_alloc_frame();
    if (!ptr) {
      abort("No free physical memory");
    }

    void *vpage = vaddr + (i * PAGE_SIZE);
    vmm_map_page(ptr, vpage, 0);
  }

  return vaddr;
}
