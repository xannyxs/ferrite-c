#include "arch/x86/gdt/gdt.h"
#include "arch/x86/entry.h"

#include <stdint.h>
#include <string.h>

extern void gdt_flush(uint32_t);
extern void *stack_top;

tss_entry_t tss_entry;
entry_t gdt_entries[6] __attribute__((section(".gdt")));
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

static void tss_init(void) {
  uint32_t base = (uint32_t)&tss_entry;
  uint32_t limit = base + sizeof(tss_entry) - 1;

  gdt_set_gate(5, base, limit, 0x89, 0x00);
  memset(&tss_entry, 0, sizeof(tss_entry_t));

  tss_entry.ss0 = 0x10;
  tss_entry.esp0 = (uintptr_t)&stack_top;
}

/* Public */

void gdt_init(void) {
  gdt_ptr.limit = sizeof(entry_t) * 6 - 1;
  gdt_ptr.base = (uint32_t)&gdt_entries;

  gdt_set_gate(0, 0, 0, 0, 0);                // NULL Gate
  gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Kernel Code Segment
  gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Kernel Data Segment
  gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User Code Segment
  gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User Data Segment

  tss_init();

  gdt_flush((uint32_t)&gdt_ptr);
  __asm__ volatile("ltr %0" : : "r"((uint16_t)0x28));
}
