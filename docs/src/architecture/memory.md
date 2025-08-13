# Memory Allocation

> ⚠️ **Heads-up!** This section is a work-in-progress.
> I am making a lot of changes right now, but this should still give you a great overview of the general design.

Memory allocation is one of my favorite and most difficult topics.
My current approach is heavily inspired by the Linux kernel.

Here is the boot-up sequence for getting the memory allocators running, in chronological order:

1. Set up a temporary page directory.
2. Place the kernel in the higher-half of memory.
3. Parse the Multiboot header to find available memory regions.
4. Initialize `memblock`, our simple bootstrap allocator.
5. Initialize the `buddy_allocator` as the core page allocator.
6. Create the final, permanent page directory.
7. Set up `kmalloc()` to provide a clean interface for kernel allocations.
8. Allocate!

Let's walk through these steps.

-----

### The Higher Half and Initial Paging

First things first, we need to get the kernel into a known, protected location.
I place it in the **higher-half** of the virtual address space, starting at `0xC0000000`.
This is a common technique that separates the kernel's address space from the user's,
preventing conflicts. Everything below this address is reserved for user-space programs.

To make this work, I create a temporary page directory that handles the initial virtual-to-physical
address translation with a simple offset. These macros help us out:

```c
#define KERNEL_BASE 0xC0000000
#define VIRT_TO_PHYS(addr) ((addr) - KERNEL_BASE)
#define PHYS_TO_VIRT(addr) ((addr) + KERNEL_BASE)
```

With this initial mapping in place, the bootloader can jump to our `kmain()`
function, which now operates from the higher-half.

### `memblock`: The Bootstrap Allocator

Before we can set up the buddy allocator, we need a way to... well, *allocate memory for the allocator\!*

This is where `memblock` comes in. After parsing the Multiboot header to see how
much RAM we have and which parts are usable, we initialize `memblock`. It is a very simple **bump allocator**:

* It keeps a pointer to the start of a free memory region.
* When you ask for memory, it returns the current pointer and "bumps" it forward by the amount you requested.

The major limitation is that there is no `free()`! Once memory is allocated with `memblock`,
it is allocated for good. It is a tool for the earliest stages of boot-up,
used only until the buddy allocator is ready.

### `buddy_allocator`: The Core Page Allocator

Now for the main event. The **buddy allocator** is the core of physical memory management.
It is designed to handle requests for blocks of memory that are a power-of-two number of pages
(e.g., 1 page, 2 pages, 4 pages, 16 pages).

Its main advantage is that it is very efficient at splitting large blocks and,
more importantly, merging freed blocks back together to reduce external fragmentation.

The main drawback is **internal fragmentation**. If you need 5KB of memory,
the buddy allocator will give you an 8KB block (the next power of two after 4KB), wasting 3KB.
This is a problem a `slab_allocator` can solve later on.
The buddy allocator's only job is to manage and hand out raw physical pages.

To keep track of all these blocks, the allocator is managed by a central struct that holds its entire state:

```c
typedef struct {
  uintptr_t base;                         // Start address of the memory region we manage
  size_t size;                            // Total size of this region
  struct buddy_node *free_lists[MAX_ORDER + 1]; // Array of linked lists for free blocks
  uint8_t *map;                           // Bitmap to track the state of blocks
  size_t map_size;                        // The size of our bitmap
  uint8_t max_order;                      // The largest allocation size (2^max_order)
} buddy_t;
```

In simple terms, the `free_lists` array is the most important part.
Each slot (or "order") points to a linked list of all available blocks of a specific size.
For example, `free_lists[0]` might be for 4KB blocks, `free_lists[1]` for 8KB blocks, and so on.
The `map` is a simple bitmap that helps the allocator quickly check if a block's "buddy" is also free.

### Putting It All Together: `kmalloc` and Final Paging

With our buddy allocator ready, we can finally create the permanent page directory.
Now that we know the exact memory layout, we can build a more robust mapping of our kernel's memory.

On older i386 processors that don't support the `invlpg` instruction, we have to
flush the entire Translation Lookaside Buffer (`TLB`) every time we map a new page,
which is quite inefficient!

Finally, we set up `kmalloc()`. This is the function that most of the kernel
will actually use. It acts as a friendly frontend for the memory subsystem:

1. A kernel component calls `kmalloc()` requesting a certain number of bytes.
2. `kmalloc()` figures out how many pages it needs and asks the **buddy allocator** for them.
3. The buddy allocator returns a **physical address**.
4. `kmalloc()` finds a free **virtual address** and maps it to the physical address given by the buddy.
5. It adds a small header to the allocation to keep track of its size and a `magic` number for debugging.
6. It returns the new virtual address to the caller.

The header looks like this. The `magic` number helps detect memory corruption if it gets overwritten.

```c
typedef struct block_header {
  size_t size;      // How big is this block?
  uint32_t magic;   // Is this a valid block?
} block_header_t;
```

And that's it! The kernel is now free to allocate memory.

---

### WIP

* Slab Allocator
* User-space Allocations
* Demand Paging

---

### Sources

* [Physical Page Allocation](https://www.kernel.org/doc/gorman/html/understand/understand009.html)
* [OSDev Paging](https://wiki.osdev.org/Paging)
* [Xv6 Documentation](https://pdos.csail.mit.edu/6.828/2018/xv6.html)
* [GWU OS: Memory Allocation - Slab and Buddy Allocators](https://www.youtube.com/watch?v=DRAHRJEAEso&t=2096s)
