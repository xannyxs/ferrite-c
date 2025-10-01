#include "memory/pmm.h"
#include "arch/x86/memlayout.h"
#include "arch/x86/multiboot.h"
#include "drivers/printk.h"
#include "lib/math.h"
#include "lib/stdlib.h"
#include "memory/consts.h"
#include "memory/vmm.h"
#include "types.h"

static u32 pmm_bitmap_size = 0;

/* Private */

static u32 get_total_memory(multiboot_info_t* mbd)
{
    u32 total_memory = 0;

    for (u32 i = 0; i < mbd->mmap_length; i += sizeof(multiboot_mmap_t)) {
        multiboot_mmap_t* entry = (multiboot_mmap_t*)(mbd->mmap_addr + i);
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
            u32 block_end = entry->addr_low + entry->len_low;
            if (block_end > total_memory) {
                total_memory = block_end;
            }
        }
    }

    return total_memory;
}

/* Public */

void pmm_print_bit(u32 addr)
{
    u32 i = addr / PAGE_SIZE;
    u32 byte_index = i / 8;
    u8 bit_index = i % 8;

    u8 data_byte = pmm_bitmap[byte_index];

    if ((data_byte >> bit_index) & 1) {
        printk("Bit at 0x%x is: 1\n", addr);
    } else {
        printk("Bit at 0x%x is: 0\n", addr);
    }
}

void pmm_print_bitmap(void)
{
    printk("--- PMM Bitmap State ---\n");
    u32 i = 0;

    for (; i < pmm_bitmap_size; i++) {
        u8 byte = pmm_bitmap[i];

        for (s32 j = 7; j >= 0; j--) {
            if ((byte >> j) & 1) {
                printk("1");
            } else {
                printk("0");
            }
        }
        printk(" ");
        if ((i + 1) % 16 == 0) {
            printk("\n\n");
        }
    }

    printk("\n------------------------\n");

    printk("Amount of bytes: %d\n", pmm_bitmap_size);
}

u32 pmm_bitmap_len(void) { return pmm_bitmap_size; }

u32 pmm_get_first_addr(void)
{
    for (size_t i = 0; i < pmm_bitmap_size; i++) {
        if (pmm_bitmap[i] == 0xFF) {
            continue;
        }

        for (int bit = 0; bit < 8; bit++) {
            if ((pmm_bitmap[i] & (1 << bit)) == 0) {
                size_t page_number = (i * 8) + bit;

                return page_number * PAGE_SIZE;
            }
        }
    }
    // Indicates out of memory. Should we abort()?
    return 0;
}

void* pmm_get_physaddr(void* vaddr)
{
    u32 pdindex = (u32)vaddr >> 22;
    u32 ptindex = (u32)vaddr >> 12 & 0x03FF;
    u32 offset = (u32)vaddr & 0xFFF;
    const u32* pd = (u32*)0xFFFFF000;

    if (!(pd[pdindex] & PTE_P)) {
        return 0;
    }

    u32* pt = (u32*)(0xFFC00000 + (pdindex * PAGE_SIZE));
    if (!(pt[ptindex] & PTE_P)) {
        return 0;
    }

    u32 phys_frame_addr = pt[ptindex] & ~0xFFF;

    return (void*)(phys_frame_addr + offset);
}

void pmm_init_from_map(multiboot_info_t* mbd)
{
    u32 total_memory = get_total_memory(mbd);

    pmm_bitmap_size = CEIL_DIV(total_memory / PAGE_SIZE, 8);
    u32 bitmap_end_vaddr = (u32)&pmm_bitmap + pmm_bitmap_size;

    u32 first_free_page_vaddr = ALIGN(bitmap_end_vaddr, PAGE_SIZE);
    u32 first_free_page_paddr = V2P_WO(first_free_page_vaddr);

    u32 kernel_end = V2P_WO((u32)&_kernel_end);
    printk("Kernel physical end: 0x%x\n", kernel_end);
    printk("first free page: 0x%x\n", first_free_page_paddr);

    for (size_t i = 0; i < pmm_bitmap_size; i++) {
        pmm_bitmap[i] = 0xFF;
    }

    for (u32 i = 0; i < mbd->mmap_length; i += sizeof(multiboot_mmap_t)) {
        multiboot_mmap_t* entry = (multiboot_mmap_t*)(mbd->mmap_addr + i);

        if (entry->type != MULTIBOOT_MEMORY_AVAILABLE) {
            continue;
        }

        printk("Start Addr Low: 0x%x | Length Low: 0x%x\n", entry->addr_low,
            entry->len_low);

        for (u32 p = 0; p < entry->len_low; p += PAGE_SIZE) {
            u32 current_addr = entry->addr_low + p;

            if (current_addr >= first_free_page_paddr) {
                pmm_clear_bit(current_addr);
            }
        }
    }
}
