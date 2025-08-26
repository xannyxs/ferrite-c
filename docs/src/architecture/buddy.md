# The Buddy Allocator

Explaining memory management can be tricky, so I've created this document to
break down the algorithm: the **Buddy Allocator**.

### What is the Buddy Allocator?

The Buddy Allocator is a memory management algorithm that takes charge of a large,
contiguous region of physical memory.
It acts like a librarian for your computer's RAM,
handing out blocks of memory when requested and tidying them up when they're returned.
As a core kernel component, it works directly with **physical addresses**,
the actual hardware addresses of your RAM.

### Why "Buddy"?

The algorithm gets its name from how it manages memory. It works on a simple principle: to get a small block of memory, you take a large block and split it in half. These two halves are now "buddies." This splitting process continues until a block of the perfect size is created.

When a block is freed, the allocator checks to see if its buddy is also free. If it is, they are instantly merged back into their original, larger block. This buddy system is a clever way to combat memory fragmentation, ensuring that large, contiguous blocks of memory are available when needed.

## The Struct

Our Buddy Allocator has the following two structs:

```c
#define PAGE_SIZE 4096 // The size of a single page, the smallest allocation unit.
#define MAX_ORDER 32   // The theoretical maximum order. The actual max is calculated at runtime.
#define MIN_ORDER 0    // The smallest order, representing a single page (2^0 pages).

// A node in a free list, stored within the free block itself.
typedef struct buddy_node {
  struct buddy_node *next;
} buddy_node_t;

typedef struct {
  uintptr_t base; // Starting physical address of the managed memory.
  size_t size;    // Total size of the managed memory in bytes.
  buddy_node_t *free_lists[MAX_ORDER + 1]; // Array of free lists, indexed by block order.
  uint8_t *map;      // Bitmap to track block states for efficient merging.
  size_t map_size;   // Size of the state bitmap in bytes.
  uint8_t max_order; // Actual max order, calculated from the total memory size.
} buddy_allocator_t;
```

My kernel creates a pre-defined buddy struct in the kernel that will manage
the whole available physical memory.

### The Math

The buddy allocator doesn't work with page counts directly.
Instead, it uses a concept called **order** to manage block sizes.
The order is simply the exponent in a power-of-two calculation.

The number of pages in a block is calculated as $2^{\text{order}}$.

* **Order 0:** $2^0 = 1$ page
* **Order 1:** $2^1 = 2$ pages
* **Order 2:** $2^2 = 4$ pages
* **Order 3:** $2^3 = 8$ pages

To find the maximum order for a given amount of memory,
we use the inverse of this formula: the base-2 logarithm.

$$\text{max\_order} = \lfloor \log_2(\text{total\_pages}) \rfloor$$

**An example**

Let's say we have **4 MiB** of available memory and a page size of **4 KiB** (4096 bytes).

1. **Calculate Total Pages:**
    $$\text{total\_pages} = \frac{4 \text{ MiB}}{4 \text{ KiB}} = \frac{4 \times 1024 \times 1024}{4096} = \frac{4,194,304}{4096} = 1024 \text{ pages}$$

2. **Calculate the Order:**
    $$\text{max\_order} = \lfloor \log_2(1024) \rfloor = 10$$

This tells us that our 4 MiB memory region can be managed as a single, top-level block of **order 10**.

## On intialisation

Each variable in the struct needs to be initialised. The memory layout will be as follows:

```
Low Address
------------------------------------
| buddy_allocator_t struct         |
|----------------------------------|
| Bitmap                           |  <-- Metadata
|----------------------------------|
| (Optional padding for alignment) |
|----------------------------------|
| Managed Memory Region            |  <-- buddy_allocator_t->base points here
| ...                              |
| ...                              |
------------------------------------
High Address
```

As you can see, the struct and bitmap of the Buddy Allocator are located
before the usable memory region that the buddy allocator will use.
The start of this memory region is represented by `base`.

### The Bitmap

At the heart of my buddy allocator is a simple but powerful data structure:
the **bitmap**. While the `free_lists` tell us if a block of a certain size
is available, the bitmap is important for managing the memory space efficiently,
especially when freeing memory.

The bitmap's primary job is to track the state of memory blocks to make merging buddies
incredibly fast. Every bit in the map corresponds to a single page of
physical memory.

* A `0` means the page is part of a larger, free block.
* A `1` means the page is part of a block that is currently allocated or has been split into smaller chunks.

Let's see it in action.
Imagine we have 16 pages of memory. When nothing is allocated, our bitmap is all zeroes.

**Initial State (All Free):**

```
[0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0]
```

Now, let's say a program requests a 4-page block (order 2).
We allocate pages 4, 5, 6, and 7. The bitmap is updated to mark these pages as "in-use".

**After Allocating 4 Pages:**

```
Bitmap: [0 0 0 0 1 1 1 1 0 0 0 0 0 0 0 0]
Pages:   0 1 2 3 4 5 6 7 8 9 ...
                 ^-----^
                 Allocated
```

### Merging

So, what happens when we free this 4-page block?
To keep our memory from becoming fragmented, we want to merge the newly freed
block with its "buddy" if the buddy is also free.

The "buddy" is the adjacent block of the same size.
For our block at page 4 (which is of order 2), its buddy is the block starting at page 0.

To check if we can merge, the allocator performs these steps:

1. **Identify the Buddy:** It calculates the buddy's starting address.
2. **Check the Bitmap:** It looks at the bitmap to check the buddy's status. It only needs to check the first bit of the buddy block (the bit for page 0).
3. **Make a Decision:**
      * If the buddy's bit is `1`, the buddy is still in use. We can't merge. The freed block is simply added to the `free_list`.
      * If the buddy's bit is `0`, the buddy is free. We can merge them into a single, larger 8-page block.

In our case, the bit for page 0 is `0`, so we can merge.
The allocator combines the block from pages 0-3 and our newly freed block from
pages 4-7 into one large, 8-page block.

**After Deallocation and Merging:**

```
Bitmap: [0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0]
```

The bitmap allowed us to instantly know the state of the buddy without searching
through complicated data structures.
This process is repeated up the chain, allowing small blocks to merge back
into the largest possible free chunks, keeping the memory ready for future allocations.

**Programming the bitmap:**

Programming the bitmap is straightforward. We get the start and end addresses
of the available physical memory and calculate how many pages fit within that range.
If 16 pages fit in the available physical memory it means we need to create a bitmap of 16 bits (2 bytes).

```c
  uintptr_t start_addr = (uintptr_t)get_next_free_addr();
  uintptr_t end_addr = (uintptr_t)get_heap_end_addr();

  size_t num_pages = (end_addr - start_addr) / PAGE_SIZE;
  size_t map_size_needed = CEIL_DIV(num_pages, 8);
  g_buddy.map_size = ALIGN(map_size_needed, sizeof(size_t));

  g_buddy.map = (uint8_t *)memblock(g_buddy.map_size);
  // Check if map exists
  memset(g_buddy.map, 0, g_buddy.map_size);

```

### The base & size

The `base` & `size` is the second section we will need to figure out.
With the help of the bitmap we can find the starting address for `base`.
Ensure that `base` is aligned with your `PAGE_SIZE`.
Take the `end_addr` and calculate how big your heap will be for the Buddy Allocator.

```c
  g_buddy.base = ALIGN((uintptr_t)g_buddy.map + g_buddy.map_size, PAGE_SIZE);
  g_buddy.size = end_addr - g_buddy.base;
  // Check if size is smaller than PAGE_SIZE (MIN_ORDER).
```

With the bitmap, `base`, and `size` established,
we have enough information to set up the rest of the buddy allocator.

## The max order

The default max order on my kernel is 32 (4 294 967 296 bytes),
which is impossible to reach, since we are working on a `x86_32` machine.

We calculate the `max_order` with our current amount of memory with the following formula:
$$O_{\text{max}} = \lfloor \log_2\left(\frac{S_{\text{total}}}{S_{\text{page}}}\right) \rfloor$$

If the requested allocation is over the `max_order`, it can be safely discarded.

## The `free_list`

The `free_list` is the most important part of the buddy allocator's allocation strategy.
It's structured as an array where each index corresponds to a block **order** (size).
Each element of the array is the head of a singly linked list containing all the
free blocks of that specific size. Since the blocks are free,
we cleverly use the memory of the block itself to store a pointer to the next free block,
requiring no extra memory.

**An example**

Imagine our system has just started, and all the memory is in one giant, free block.
If our maximum size is an order 9 block (`2^9 = 512` pages), our `free_list` looks like this:

```
Free List: [0, 0, 0, 0, 0, 0, 0, 0, 0, 1]
Order:      0  1  2  3  4  5  6  7  8  9
```

Now, a program asks for a tiny **order 0** block (1 page). The allocator follows these steps:

1. **Check Order 0:** It looks in `free_list[0]` and finds it empty.
2. **Find a Bigger Block:** It searches up the orders until it finds the first non-empty list,
which is at order 9.
3. **Split the Block:** The order 9 block is too big, so the allocator splits it.
This creates two "buddy" blocks of order 8. One is kept to continue the process,
and the other is placed in the order 8 free list.

```
Free List: [0, 0, 0, 0, 0, 0, 0, 0, 1, 0]
Order:      0  1  2  3  4  5  6  7  8  9
```

4. **Repeat:** This process repeats. The order 8 block is split into two order 7s.
One is kept, and the other is added to the `free_list[7]`.
This continues all the way down until we finally create two order 0 blocks.

5. **Fulfill the Request:** Once we have two order 0 blocks,
the allocator gives one to the user and places the other on the `free_list[0]`.

After fulfilling the request for one order 0 block, our `free_list` now has a
free block available at every order except order 9.

```
Free List: [1, 1, 1, 1, 1, 1, 1, 1, 1, 0]
Order:      0  1  2  3  4  5  6  7  8  9
```

This splitting mechanism ensures that even if only large blocks are free,
the allocator can efficiently generate a block of any required size.

**Implementation**

A key challenge in initializing the buddy allocator is that the total amount of
available physical memory is rarely a perfect power of two.
If we have 7 pages of memory, for example, we can't create a single 8-page block.

To handle this, the initialization code "carves" the available memory into the
largest possible power-of-two blocks that will fit.

#### The Carving Loop

We run a loop that repeatedly carves off the largest possible block from the
remaining space and adds it to the appropriate free list.
Let's trace it with an example of **7 pages** of available memory:

1. **First Iteration:**

      * `remaining` is 7 pages (28 672 bytes)
      * The largest power-of-two block that fits is 4 pages (order 2).
      * We create a 4-page block and add it to `free_list[2]`.
      * `remaining` is now `7 - 4 = 3` pages.

2. **Second Iteration:**

      * `remaining` is 3 pages (12 288 bytes)
      * The largest power-of-two block that fits is 2 pages (order 1).
      * We create a 2-page block and add it to `free_list[1]`.
      * `remaining` is now `3 - 2 = 1` page.

3. **Third Iteration:**

      * `remaining` is 1 page (4096 bytes)
      * The largest power-of-two block that fits is 1 page (order 0).
      * We create a 1-page block and add it to `free_list[0]`.
      * `remaining` is now `1 - 1 = 0` pages. The loop stops here.

After the loop, our free lists will contain one block of order 2, one of order 1,
and one of order 0, perfectly accounting for our 7 pages of memory.

Here is the C code that implements this logic:

```c
// First, ensure all free lists are empty.
for (int32_t i = 0; i <= MAX_ORDER; i++) {
    g_buddy.free_lists[i] = NULL;
}

size_t remaining = g_buddy.size;
uintptr_t current_addr = g_buddy.base;

// Loop until all available memory has been carved up.
while (remaining >= PAGE_SIZE) {
    // Find the largest order that fits in the remaining space.
    uint32_t order = floor_log2(remaining / PAGE_SIZE);

    // This check is a safeguard.
    if (order > g_buddy.max_order) {
        order = g_buddy.max_order;
    }

    // Add the new block to the head of the correct free list.
    buddy_node_t *block = (buddy_node_t *)current_addr;
    block->next = g_buddy.free_lists[order];
    g_buddy.free_lists[order] = block;

    // Subtract the carved block's size and advance our address.
    size_t block_size = PAGE_SIZE << order;
    current_addr += block_size;
    remaining -= block_size;
}
```

### Conclusion

The Buddy Allocator, with its clever use of `free_lists`, a bitmap,
and a power-of-two "order" system, stands as a simple yet powerful solution
for managing physical memory. By treating adjacent memory blocks as "buddies"
that can be split and merged, it efficiently provides memory of various sizes
while effectively combating fragmentation.
