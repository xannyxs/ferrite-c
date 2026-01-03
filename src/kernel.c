#include "arch/x86/gdt/gdt.h"
#include "arch/x86/idt/idt.h"
#include "arch/x86/io.h"
#include "arch/x86/pic.h"
#include "arch/x86/pit.h"
#include "arch/x86/time/rtc.h"
#include "drivers/block/ide.h"
#include "drivers/video/vga.h"
#include "fs/mount.h"
#include "fs/vfs.h"
#include "memory/buddy_allocator/buddy.h"
#include "memory/memblock.h"
#include "memory/pmm.h"
#include "memory/vmalloc.h"
#include "memory/vmm.h"
#include "sys/process/process.h"

#include <ferrite/types.h>
#include <lib/stdlib.h>
#include <stdbool.h>

__attribute__((noreturn)) void kmain(u32 magic, multiboot_info_t* mbd)
{

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        abort("Invalid magic number!");
    }

    if (!(mbd->flags & (1 << 2))) {
        abort("Missing cmdline flag. Cannot locate root devices");
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

    inode_cache_init();
    ide_init();
    // FUTURE: Will add other type of devices

    mount_root_device((char*)mbd->cmdline);
    vfs_init();

    ptables_init();

    sti();

    create_initial_process();
    schedule();
    __builtin_unreachable();
}
