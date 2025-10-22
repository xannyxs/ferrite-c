#include "arch/x86/gdt/gdt.h"
#include "arch/x86/idt/idt.h"
#include "arch/x86/io.h"
#include "arch/x86/pic.h"
#include "arch/x86/pit.h"
#include "arch/x86/time/rtc.h"
#include "drivers/block/ide.h"
#include "drivers/video/vga.h"
#include "lib/stdlib.h"
#include "memory/buddy_allocator/buddy.h"
#include "memory/memblock.h"
#include "memory/pmm.h"
#include "memory/vmalloc.h"
#include "memory/vmm.h"
#include "sys/process/process.h"
#include "types.h"

#include <stdbool.h>

#if !defined(__i386__)
#    error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif

__attribute__((noreturn)) void kmain(u32 magic, multiboot_info_t* mbd)
{
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        abort("Invalid magic number!");
    }

    gdt_init();
    idt_init();
    pic_remap(0x20, 0x28);
    set_pit_count(LATCH);

    vga_init();
    rtc_init();

    pmm_init_from_map(mbd);
    memblock_init();

    vmm_init_pages();
    buddy_init();
    memblock_deactivate();
    vmalloc_init();

    ide_init();

    init_ptables();

    sti();

    create_initial_process();
    schedule();
    __builtin_unreachable();
}
