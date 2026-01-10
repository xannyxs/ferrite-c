#include "memory/buddy_allocator/buddy.h"
#include "arch/x86/memlayout.h"
#include "debug/debug.h"
#include "drivers/printk.h"
#include "lib/math.h"
#include "memory/consts.h"
#include "memory/memblock.h"
#include "memory/vmm.h"

#include <ferrite/string.h>
#include <ferrite/types.h>
#include <lib/stdlib.h>
#include <stdbool.h>

static buddy_allocator_t g_buddy = { 0 };

/* Private */

static int buddy_get_bit(int bit_index)
{
    u32 byte_index = bit_index / 8;
    u32 bit_in_byte = bit_index % 8;

    if (byte_index >= g_buddy.map_size) {
        return 1;
    }

    return (g_buddy.map[byte_index] & (1 << bit_in_byte)) ? 1 : 0;
}

void buddy_visualize(void)
{
    printk("\n--- Buddy Allocator Visualization ---\n");
    printk(
        "  Base Address: 0x%x | Total Size: %u KB | Max Order: %u\n",
        (void*)g_buddy.base, g_buddy.size / 1024, g_buddy.max_order
    );
    printk("-------------------------------------\n");

    printk("  Free Lists by Order:\n");
    for (int i = 0; i <= g_buddy.max_order; i++) {
        buddy_node_t* node = g_buddy.free_lists[i];
        int free_block_count = 0;
        while (node) {
            free_block_count++;
            node = node->next;
        }
        int pages_per_block = 1 << i;
        printk(
            "    Order %d (%d pages): %d free blocks\n", i, pages_per_block,
            free_block_count
        );
    }
    printk("-------------------------------------\n");

    printk("  Allocation Bitmap ('X' = allocated, '.' = free):\n");
    size_t total_pages = g_buddy.size / PAGE_SIZE;
    int bits_per_line = 32;

    for (size_t i = 0; i < total_pages; i++) {
        if (i % bits_per_line == 0) {
            if (i > 0) {
                printk("\n");
            }
            printk("    [%d-%d] ", i, i + bits_per_line - 1);
        }

        printk("%c", buddy_get_bit(i) ? 'X' : '.');
    }

    printk("\n-------------------------------------\n");
}

static bool is_block_free(u32 i, u32 order)
{
    u32 blocks_to_check = 1 << order;

    for (u32 j = 0; j < blocks_to_check; j++) {
        u32 current_page_index = i + j;
        u32 byte_index = current_page_index / 8;
        u32 bit_in_byte = current_page_index % 8;

        if (byte_index >= g_buddy.map_size) {
            return false;
        }

        if ((g_buddy.map[byte_index] & (1 << bit_in_byte)) != 0) {
            return false;
        }
    }

    return true;
}

static bool remove_from_free_list(vaddr_t vaddr, u32 order)
{
    if (order > g_buddy.max_order) {
        abort("Invalid order provided to remove_from_free_list");
    }

    buddy_node_t* target = (buddy_node_t*)vaddr;
    buddy_node_t* head = g_buddy.free_lists[order];

    if (head == target) {
        g_buddy.free_lists[order] = head->next;
        return true;
    }

    buddy_node_t* prev = head;
    buddy_node_t* current = head->next;

    while (current) {
        if (current == target) {
            prev->next = current->next;
            return true;
        }
        prev = current;
        current = current->next;
    }

    return false;
}

static void mark_free(vaddr_t vaddr, u32 order)
{
    u32 i = (vaddr - g_buddy.base) / PAGE_SIZE;
    u32 blocks_to_mark = 1 << order;

    for (u32 j = 0; j < blocks_to_mark; j++) {
        u32 current = i + j;
        u32 byte_index = current / 8;
        u32 bit_index = current % 8;

        if (byte_index >= g_buddy.map_size) {
            abort("Deallocation address out of map bounds!");
        }

        g_buddy.map[byte_index] &= ~(1 << bit_index);
    }
}

static void mark_allocated(vaddr_t vaddr, u32 order)
{
    u32 i = (vaddr - g_buddy.base) / PAGE_SIZE;
    u32 blocks_to_mark = 1 << order;

    for (u32 j = 0; j < blocks_to_mark; j++) {
        u32 current = i + j;
        u32 byte_index = current / 8;
        u32 bit_in_byte = current % 8;

        if (byte_index >= g_buddy.map_size) {
            abort("Allocation address out of map bounds!");
        }

        g_buddy.map[byte_index] |= (1 << bit_in_byte);
    }
}

static inline void buddy_list_add(vaddr_t vaddr, u32 k)
{
    buddy_node_t* node = (buddy_node_t*)vaddr;
    node->next = g_buddy.free_lists[k];
    g_buddy.free_lists[k] = node;
}

/* Public */

u32 buddy_get_max_order(void) { return g_buddy.max_order; }

size_t buddy_get_total_memory(void) { return g_buddy.size; }

void buddy_dealloc(paddr_t paddr, u32 order)
{
    vaddr_t vaddr = P2V_WO(paddr);
    mark_free(vaddr, order);

    paddr_t current_paddr = paddr;
    u32 current_order = order;

    while (current_order < g_buddy.max_order) {
        size_t block_size = PAGE_SIZE << current_order;
        paddr_t buddy_paddr = current_paddr ^ block_size;
        vaddr_t buddy_vaddr = P2V_WO(buddy_paddr);
        u32 buddy_index = (buddy_vaddr - g_buddy.base) / PAGE_SIZE;

        if (!is_block_free(buddy_index, current_order)) {
            break;
        }

        if (!remove_from_free_list(buddy_vaddr, current_order)) {
            break;
        }

        mark_allocated(buddy_vaddr, current_order);

        current_paddr
            = (current_paddr < buddy_paddr) ? current_paddr : buddy_paddr;
        current_order += 1;
    }

    vaddr_t current_vaddr = P2V_WO(current_paddr);
    buddy_list_add(current_vaddr, current_order);
}

void* buddy_alloc(u32 order)
{
    if (order > g_buddy.max_order) {
        return NULL;
    }

    u32 k = order;
    while (k <= g_buddy.max_order && !g_buddy.free_lists[k]) {
        k += 1;
    }

    if (!g_buddy.free_lists[k]) {
        printk("Out of memory. No suitable block found.\n");
        return NULL;
    }

    buddy_node_t* node = g_buddy.free_lists[k];
    vaddr_t block_vaddr = (vaddr_t)node;
    g_buddy.free_lists[k] = node->next;

    while (k > order) {
        k -= 1;
        vaddr_t buddy_vaddr = block_vaddr + (PAGE_SIZE << k);

        buddy_list_add(buddy_vaddr, k);
    }

    mark_allocated(block_vaddr, order);
    return (void*)V2P_WO(block_vaddr);
}

void buddy_init(void)
{
    paddr_t start_paddr = (paddr_t)get_next_free_addr();
    paddr_t end_paddr = (paddr_t)get_heap_end_addr();
    vaddr_t end_vaddr = P2V_WO(end_paddr);

    size_t num_pages = (end_paddr - start_paddr) / PAGE_SIZE;
    size_t map_size_needed = CEIL_DIV(num_pages, 8);
    g_buddy.map_size = ALIGN(map_size_needed, sizeof(size_t));

    paddr_t map_ptr = (paddr_t)memblock(g_buddy.map_size);
    g_buddy.map = (u8*)P2V_WO(map_ptr);
    if (!g_buddy.map) {
        abort("Could not allocate the bitmap for Buddy Allocator");
    }
    memset(g_buddy.map, 0, g_buddy.map_size);

    vaddr_t temp_base = ALIGN((u32)g_buddy.map + g_buddy.map_size, PAGE_SIZE);
    size_t temp_size = end_vaddr - temp_base;
    u32 temp_max_order = floor_log2(temp_size / PAGE_SIZE);

    g_buddy.base = ALIGN(
        (u32)g_buddy.map + g_buddy.map_size, PAGE_SIZE << temp_max_order
    );
    g_buddy.size = end_vaddr - g_buddy.base;
    if (g_buddy.size < PAGE_SIZE) {
        abort("Not enough memory for the buddy pool");
    }

    g_buddy.max_order = floor_log2(g_buddy.size / PAGE_SIZE);

    for (s32 i = 0; i <= MAX_ORDER; i++) {
        g_buddy.free_lists[i] = NULL;
    }

    size_t remaining = g_buddy.size;
    vaddr_t current_addr = g_buddy.base;

    while (remaining >= PAGE_SIZE) {
        u32 order = floor_log2(remaining / PAGE_SIZE);
        if (order > g_buddy.max_order) {
            order = g_buddy.max_order;
        }

        vmm_remap_page(SCRATCH_VADDR, (void*)V2P_WO(current_addr), 0);
        buddy_node_t* block = (buddy_node_t*)SCRATCH_VADDR;

        block->next = g_buddy.free_lists[order];
        g_buddy.free_lists[order] = (buddy_node_t*)current_addr;

        size_t block_size = PAGE_SIZE << order;
        current_addr += block_size;
        remaining -= block_size;
    }
}
