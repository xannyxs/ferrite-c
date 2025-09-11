#include "arch/x86/gdt/gdt.h"
#include "arch/x86/entry.h"
#include "sys/cpu.h"

#include <stdint.h>
#include <string.h>

extern void gdt_flush(uint32_t);
extern void *stack_top;
extern int32_t ncpu;
extern cpu_t cpus[];

static entry_t gdt_entries[NUM_ENTRIES];
static descriptor_pointer_t gdt_ptr;

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

static void tss_init(int32_t num, tss_entry_t *entry) {
  memset(entry, 0, sizeof(tss_entry_t));

  entry->ss0 = 0x10;
  entry->esp0 = 0;

  gdt_set_gate(num, (uint32_t)entry, sizeof(tss_entry_t) - 1, 0x89, 0x00);
}

/* Public */

void gdt_init(void) {
  gdt_ptr.limit = sizeof(entry_t) * NUM_ENTRIES - 1;
  gdt_ptr.base = (uint32_t)&gdt_entries;

  gdt_set_gate(0, 0, 0, 0, 0);                // NULL Gate
  gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Kernel Code Segment
  gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Kernel Data Segment
  gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User Code Segment
  gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User Data Segment

  tss_init(5, &cpus[0].ts);

  gdt_flush((uint32_t)&gdt_ptr);
  __asm__ volatile("ltr %0" : : "r"((uint16_t)0x28));
}
