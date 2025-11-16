#ifndef GDT_H
#define GDT_H

#include <ferrite/types.h>

typedef struct {
    u32 prev_tss;
    u32 esp0, ss0;
    u32 esp1, ss1;
    u32 esp2, ss2;

    u32 cr3;
    u32 eip, eflags;
    u32 eax, ecx, edx, ebx;
    u32 esp, ebp, esi, edi;
    u32 es, cs, ss, ds, fs, gs;

    u32 ldt;
    u16 trap, iomap_base;
} __attribute__((packed)) tss_entry_t;

void tss_set_stack(u32 stack);

void gdt_init(void);

#endif /* GDT_H */
