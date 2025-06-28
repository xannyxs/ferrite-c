#ifndef VMM_H
#define VMM_H

#include <stddef.h>
#include <stdint.h>

#define VMM_START 0xD0000000

#define ALIGN(x, a) __ALIGN_MASK(x, (typeof(x))(a) - 1)
#define __ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))

static uint32_t kernel_heap_top = VMM_START;
static uint32_t kernel_heap_mapped_size = 0;

void vmm_expand_heap(size_t);

#endif /* VMM_H */
