#ifndef GDT_H
#define GDT_H

#include <stdint.h>

typedef struct gdt_entry {
  uint16_t limit;
  uint16_t lower_base;
  uint8_t middle_base;
  uint8_t access;
  uint8_t flags;
  uint8_t higher_base;
} __attribute__((packed)) gdt_entry_t;

typedef struct gdt_pointer {
  uint16_t limit;
  uint32_t base;

} __attribute__((packed)) gdt_pointer_t;

void gdt_init();

#endif /* GDT_H */
