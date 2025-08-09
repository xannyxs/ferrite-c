#include "lib/math.h"
#include "memory/memblock.h"
#include "memory/pmm.h"
#include "memory/vmm.h"

#include <stddef.h>
#include <stdint.h>

static void *heap_current_break = NULL;

void kmem_init(void *current_break) { heap_current_break = current_break; }

void *get_current_break(void) { return heap_current_break; }

void *kbrk(void *addr) {
  void *aligned_new_break = (void *)ALIGN((uint32_t)addr, PAGE_SIZE);

  while (heap_current_break < aligned_new_break) {
    void *ptr = (void *)pmm_alloc_frame();
    if (!ptr) {
      return heap_current_break;
    }

    vmm_map_page(ptr, heap_current_break, 0);
    heap_current_break = (void *)((uint32_t)heap_current_break + PAGE_SIZE);
  }

  heap_current_break = aligned_new_break;

  return heap_current_break;
}
