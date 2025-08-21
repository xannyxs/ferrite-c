#ifndef GDT_H
#define GDT_H

#include <stdint.h>

typedef struct {
  uint32_t prev_tss;
  uint32_t esp0, ss0;

  uint32_t esp1, ss1;
  uint32_t esp2, ss2;

  uint32_t cr3;
  uint32_t eip;
  uint32_t eflags;
  uint32_t eax, ecx;
  uint32_t edx, ebx;
  uint32_t esp, ebp, esi, edi;

  uint32_t es, cs, ss, ds, fs, gs;

  uint32_t ldt;
  uint32_t trap;
  uint32_t io_map_base;
} __attribute__((packed)) tss_entry_t;

void gdt_init(void);

#endif /* GDT_H */
