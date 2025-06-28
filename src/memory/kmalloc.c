#include "debug/debug.h"
#include "vmm.h"

#include <stddef.h>
#include <stdint.h>

void *kmalloc(size_t size) {
  if (size == 0) {
    return NULL;
  }

  size = ALIGN(size, 4);

  void *allocation_ptr = (void *)kernel_heap_top;
  uint32_t new_top = kernel_heap_top + size;

  if (new_top > VMM_START + kernel_heap_mapped_size) {
    uint32_t required_size = new_top - VMM_START;
    bochs_print_string("Before expanding\n");
    BOCHS_MAGICBREAK();
    vmm_expand_heap(required_size);
  }

  kernel_heap_top = new_top;

  return (void *)allocation_ptr;
}
