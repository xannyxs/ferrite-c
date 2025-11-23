#include "arch/x86/gdt/gdt.h"
#include "arch/x86/entry.h"
#include "lib/string.h"
#include <ferrite/types.h>

#define NUM_ENTRIES 6

extern void gdt_flush(u32);
extern void* stack_top;

static tss_entry_t tss_entry;
static entry_t gdt_entries[NUM_ENTRIES];
static descriptor_pointer_t gdt_ptr;

/* Private */

static void gdt_set_gate(u32 num, u32 base, u32 limit, u8 access, u8 gran)
{
    gdt_entries[num].lower_base = (base & 0xFFFF);
    gdt_entries[num].middle_base = (base >> 16) & 0xFF;
    gdt_entries[num].higher_base = (base >> 24) & 0xFF;

    gdt_entries[num].limit = (limit & 0xFFFF);
    gdt_entries[num].flags = (limit >> 16) & 0x0F;
    gdt_entries[num].flags |= (gran & 0xF0);

    gdt_entries[num].access = access;
}

static void tss_init(void)
{
    gdt_set_gate(5, (u32)&tss_entry, sizeof(tss_entry_t) - 1, 0x89, 0x00);
    memset(&tss_entry, 0, sizeof(tss_entry_t));

    tss_entry.ss0 = 0x10;
    tss_entry.esp0 = (u32)&stack_top;
}

/* Public */

void tss_set_stack(u32 stack) { tss_entry.esp0 = stack; }

void gdt_init(void)
{
    gdt_ptr.limit = sizeof(entry_t) * NUM_ENTRIES - 1;
    gdt_ptr.base = (u32)&gdt_entries;

    gdt_set_gate(0, 0, 0, 0, 0);                // NULL Gate
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Kernel Code Segment
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Kernel Data Segment
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User Code Segment
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User Data Segment

    tss_init();

    gdt_flush((u32)&gdt_ptr);
    __asm__ volatile("ltr %0" : : "r"((u16)0x28));
}
