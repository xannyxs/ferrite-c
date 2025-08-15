#ifndef GDT_H
#define GDT_H

#include <stdint.h>

typedef struct {
  uint32_t prev_tss;
  uint32_t esp0;

  uint32_t ss0;
  uint32_t esp1;
  uint32_t ss1;
  uint32_t esp2;
  uint32_t ss2;

  uint32_t cr3, eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
  uint32_t es, cs, ss, ds, fs, gs;
  uint32_t ldt;
  uint16_t trap, iomap_base;
} __attribute__((packed)) tss_entry_t;

void gdt_init(void);

#endif /* GDT_H */
