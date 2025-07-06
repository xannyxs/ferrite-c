#include "arch/x86/entry.h"

#include <stdint.h>

extern void gdt_flush(uint32_t);

entry_t gdt_entries[5] __attribute__((section(".gdt")));
descriptor_pointer_t gdt_ptr;

/* Private */

static void gdt_set_gate(uint32_t num, uint32_t base, uint32_t limit,
                         uint8_t access, uint8_t gran) {
  gdt_entries[num].lower_base = (base & 0xFFFF);
  gdt_entries[num].middle_base = (base >> 16) & 0xFF;
  gdt_entries[num].higher_base = (base >> 24) & 0xFF;

  gdt_entries[num].limit = (limit & 0xFFFF);
  gdt_entries[num].flags = (limit >> 16) & 0x0F;
  gdt_entries[num].flags |= (gran & 0xF0);

  gdt_entries[num].access = access;
}

/* Public */

void gdt_init() {
  gdt_ptr.limit = sizeof(entry_t) * 5 - 1;
  gdt_ptr.base = (uint32_t)&gdt_entries;

  gdt_set_gate(0, 0, 0, 0, 0);                            // NULL Gate
  gdt_set_gate(1, 0, 0xFFFFFFFF, 0b10011010, 0b11001111); // Kernel Code Segment
  gdt_set_gate(2, 0, 0xFFFFFFFF, 0b10010010, 0b11001111); // Kernel Data Segment
  gdt_set_gate(3, 0, 0xFFFFFFFF, 0b11111010, 0b11001111); // User Code Segment
  gdt_set_gate(4, 0, 0xFFFFFFFF, 0b11110010, 0b11001111); // User Data Segment

  gdt_flush((uint32_t)&gdt_ptr);
}
