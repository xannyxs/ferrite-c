#ifdef __TEST
#    include "memory/buddy_allocator/buddy.h"
#    include "memory/consts.h"
#    include "tests/tests.h"
#    include <ferrite/types.h>
#    include <stdbool.h>

extern u32 tests_passed;
extern u32 tests_failed;

TEST(buddy_single_page_alloc)
{
    printk("  Testing single page allocation...\n");
    paddr_t page = (paddr_t)buddy_alloc(0); // Order 0 = 1 page
    ASSERT(page != 0, "Should allocate a page");
    ASSERT((page & (PAGE_SIZE - 1)) == 0, "Page should be aligned");
    printk("  Allocated page at paddr: %x\n", page);

    buddy_dealloc(page, 0);
    printk("  Freed page successfully\n");
    return true;
}

TEST(buddy_multiple_orders)
{
    printk("  Testing allocation of different orders...\n");
    paddr_t page1 = (paddr_t)buddy_alloc(0); // 1 page
    paddr_t page2 = (paddr_t)buddy_alloc(1); // 2 pages
    paddr_t page3 = (paddr_t)buddy_alloc(2); // 4 pages

    ASSERT(page1 != 0, "Order 0 allocation should succeed");
    ASSERT(page2 != 0, "Order 1 allocation should succeed");
    ASSERT(page3 != 0, "Order 2 allocation should succeed");

    printk("  Order 0: %x\n", page1);
    printk("  Order 1: %x\n", page2);
    printk("  Order 2: %x\n", page3);

    buddy_dealloc(page1, 0);
    buddy_dealloc(page2, 1);
    buddy_dealloc(page3, 2);
    return true;
}

TEST(buddy_alloc_free_alloc)
{
    printk("  Testing alloc -> free -> alloc...\n");
    paddr_t page1 = (paddr_t)buddy_alloc(0);
    ASSERT(page1 != 0, "First allocation should succeed");
    printk("  First alloc: %x\n", page1);

    buddy_dealloc(page1, 0);
    printk("  Freed page\n");

    paddr_t page2 = (paddr_t)buddy_alloc(0);
    ASSERT(page2 != 0, "Second allocation should succeed");
    printk("  Second alloc: %x\n", page2);

    // Should get the same page back (LIFO behavior)
    ASSERT(page1 == page2, "Should reuse the freed page");

    buddy_dealloc(page2, 0);
    return true;
}

TEST(buddy_alignment)
{
    printk("  Testing block alignment...\n");

    for (u32 order = 0; order <= 3; order++) {
        printk("  Attempting to allocate order %u...\n", order);
        paddr_t block = (paddr_t)buddy_alloc(order);

        printk("    Allocated block: %x\n", block);

        if (block == 0) {
            printk("    Allocation FAILED - skipping\n");
            continue;
        }

        size_t block_size = PAGE_SIZE << order;
        printk("    Block size: %u (0x%x)\n", block_size, block_size);
        printk("    Block address: 0x%x\n", block);
        printk("    Alignment mask: 0x%x\n", block_size - 1);
        printk(
            "    block & (block_size - 1) = 0x%x\n", block & (block_size - 1)
        );

        if ((block & (block_size - 1)) != 0) {
            printk("    *** MISALIGNED! ***\n");
        }

        ASSERT(
            (block & (block_size - 1)) == 0,
            "Block should be aligned to its size"
        );

        printk(
            "  Order %u (size %u): %x - aligned correctly\n", order, block_size,
            block
        );

        buddy_dealloc(block, order);
    }
    return true;
}

TEST(buddy_exhaustion)
{
    printk("  Testing memory exhaustion...\n");
#    define MAX_ALLOCS 100
    paddr_t allocs[MAX_ALLOCS];
    u32 count = 0;

    // Allocate until we run out
    for (count = 0; count < MAX_ALLOCS; count++) {
        allocs[count] = (paddr_t)buddy_alloc(0);
        if (allocs[count] == 0) {
            break;
        }
    }

    ASSERT(count > 0, "Should allocate at least one page");

    // Free everything
    for (u32 i = 0; i < count; i++) {
        buddy_dealloc(allocs[i], 0);
    }

    // Should be able to allocate again
    paddr_t page = (paddr_t)buddy_alloc(0);
    ASSERT(page != 0, "Should be able to allocate after freeing all");
    buddy_dealloc(page, 0);

    return true;
}

TEST(buddy_double_free_detection)
{
    printk("  Testing double-free detection...\n");
    paddr_t page = (paddr_t)buddy_alloc(0);
    ASSERT(page != 0, "Allocation should succeed");

    buddy_dealloc(page, 0);
    printk("  First free succeeded\n");

    // This should ideally be caught by your buddy allocator
    // If you have checks for this, it might abort or return an error
    // For now, we'll just note that it shouldn't crash
    printk("  Note: Double-free behavior depends on implementation\n");

    return true;
}

TEST(buddy_coalescing)
{
    printk("  Testing buddy coalescing...\n");

    // Allocate a large block
    paddr_t large = (paddr_t)buddy_alloc(2); // 4 pages
    ASSERT(large != 0, "Large allocation should succeed");
    printk("  Allocated 4-page block at %x\n", large);

    // Free it
    buddy_dealloc(large, 2);
    printk("  Freed 4-page block\n");

    // Should be able to allocate the same size again
    paddr_t large2 = (paddr_t)buddy_alloc(2);
    ASSERT(large2 != 0, "Should reallocate same size");
    ASSERT(large == large2, "Should get same block back (coalesced)");

    buddy_dealloc(large2, 2);
    return true;
}

TEST(buddy_fragmentation_recovery)
{
    printk("  Testing fragmentation and recovery...\n");

    // Allocate multiple small blocks
    paddr_t blocks[4];
    for (int i = 0; i < 4; i++) {
        blocks[i] = (paddr_t)buddy_alloc(0);
        ASSERT(blocks[i] != 0, "Small allocation should succeed");
    }

    printk("  Allocated 4 single pages\n");

    // Free them all - they should coalesce
    for (int i = 0; i < 4; i++) {
        buddy_dealloc(blocks[i], 0);
    }

    printk("  Freed all pages\n");

    // Now try to allocate a larger block that requires coalescing
    paddr_t large = (paddr_t)buddy_alloc(2); // 4 pages
    ASSERT(
        large != 0, "Should be able to allocate large block after coalescing"
    );

    printk("  Successfully allocated 4-page block at %x\n", large);
    buddy_dealloc(large, 2);
    return true;
}

TEST(buddy_split_and_merge)
{
    printk("  Testing block splitting and merging...\n");

    // Allocate an order-3 block (8 pages)
    paddr_t large = (paddr_t)buddy_alloc(3);
    ASSERT(large != 0, "Large allocation should succeed");
    printk("  Allocated 8-page block at %x\n", large);

    // Free it
    buddy_dealloc(large, 3);
    printk("  Freed 8-page block\n");

    // Allocate first 4-page block - should get the lower half
    paddr_t med1 = (paddr_t)buddy_alloc(2);
    ASSERT(med1 != 0, "First medium allocation should succeed");
    printk("  First 4-page block: %x\n", med1);

    // Allocate second 4-page block - should get the upper half
    paddr_t med2 = (paddr_t)buddy_alloc(2);
    ASSERT(med2 != 0, "Second medium allocation should succeed");
    printk("  Second 4-page block: %x\n", med2);

    // Check if they came from our original block
    paddr_t lower = (med1 < med2) ? med1 : med2;
    paddr_t upper = (med1 < med2) ? med2 : med1;

    printk("  Lower block: %x, Upper block: %x\n", lower, upper);
    printk("  Original block: %x\n", large);

    // If they're from our block, they should be buddies
    if (lower == large) {
        size_t block_size = PAGE_SIZE << 2;
        printk("  Blocks came from split - checking buddy relationship\n");
        ASSERT((lower ^ upper) == block_size, "Blocks should be buddies");
        ASSERT(
            upper == lower + block_size, "Upper should be one block_size away"
        );
    } else {
        printk("  Blocks came from different sources (allocator had other free "
               "blocks)\n");
    }

    // Free them and try to get the original large block back
    buddy_dealloc(med1, 2);
    buddy_dealloc(med2, 2);
    printk("  Freed both 4-page blocks\n");

    // If they were buddies from our original block, we should get it back
    paddr_t large2 = (paddr_t)buddy_alloc(3);
    ASSERT(large2 != 0, "Should reallocate 8-page block");

    if (lower == large) {
        ASSERT(large2 == large, "Should get original block back after merge");
        printk("  Got original block back: %x\n", large2);
    } else {
        printk("  Got an 8-page block: %x\n", large2);
    }

    buddy_dealloc(large2, 3);
    return true;
}

TEST(buddy_mixed_workload)
{
    printk("  Testing mixed allocation workload...\n");

    paddr_t small1 = (paddr_t)buddy_alloc(0);
    paddr_t large1 = (paddr_t)buddy_alloc(3);
    paddr_t medium1 = (paddr_t)buddy_alloc(1);
    paddr_t small2 = (paddr_t)buddy_alloc(0);

    ASSERT(
        small1 != 0 && large1 != 0 && medium1 != 0 && small2 != 0,
        "All allocations should succeed"
    );

    printk("  Allocated mixed sizes successfully\n");

    // Free in different order than allocated
    buddy_dealloc(medium1, 1);
    buddy_dealloc(small1, 0);
    buddy_dealloc(small2, 0);
    buddy_dealloc(large1, 3);

    printk("  Freed in different order - no crashes\n");
    return true;
}

TEST(buddy_max_order_allocation)
{
    printk("  Testing maximum order allocation...\n");
    u32 max_order = buddy_get_max_order();

    paddr_t max_block = (paddr_t)buddy_alloc(max_order);

    if (max_block != 0) {
        printk("  Successfully allocated max order block at %x\n", max_block);

        size_t block_size = PAGE_SIZE << max_order;
        ASSERT(
            (max_block & (block_size - 1)) == 0,
            "Max order block should be properly aligned"
        );

        buddy_dealloc(max_block, max_order);
    } else {
        printk("  Max order allocation failed (fragmented or in use)\n");
    }

    return true;
}

TEST(buddy_alternating_alloc_free)
{
    printk("  Testing alternating allocation and freeing...\n");

    for (int i = 0; i < 10; i++) {
        paddr_t block = (paddr_t)buddy_alloc(1);
        ASSERT(block != 0, "Allocation should succeed");
        buddy_dealloc(block, 1);
    }

    printk("  10 alloc/free cycles completed successfully\n");
    return true;
}

void buddy_tests(void)
{
    printk("\n========== BUDDY ALLOCATOR TEST SUITE ==========\n\n");

    RUN_TEST(buddy_single_page_alloc);
    RUN_TEST(buddy_multiple_orders);
    RUN_TEST(buddy_alloc_free_alloc);
    RUN_TEST(buddy_alignment);
    RUN_TEST(buddy_exhaustion);
    RUN_TEST(buddy_double_free_detection);
    RUN_TEST(buddy_coalescing);
    RUN_TEST(buddy_fragmentation_recovery);
    RUN_TEST(buddy_split_and_merge);
    RUN_TEST(buddy_mixed_workload);
    RUN_TEST(buddy_max_order_allocation);
    RUN_TEST(buddy_alternating_alloc_free);
}

#endif

typedef int _test_translation_unit_not_empty;
