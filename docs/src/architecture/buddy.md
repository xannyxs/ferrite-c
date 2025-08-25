# The Buddy Allocator

I had a hard time explaining the Buddy Allocator to other people.
I decided to create a brief document to explain it as best as
I can with the help of drawings.

The buddy Allocator is in charge of all the available physical memory. It
only works with physical address and only returns a physical address. Simply because
it works directly with the physical memory.

The buddy allocator is called the Buddy Allocator because it creates a "Buddy" until
it can find a block that fits in the amount of pages it should return.

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

I create a pre-defined buddy struct in the kernel that will manage the whole physical memory.

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

As you can see, the struct of the buddy Allocator is before the actual usable memory regions that the buddy allocator will use.
This start of the Memory Region will be displayed by `base`.

The `base` & `size` is the first section we will figure out.
It receives two variables. Where the Available Physical memory start and where the Physical Memory ends.

```c
  uintptr_t start_addr = (uintptr_t)get_next_free_addr();
  uintptr_t end_addr = (uintptr_t)get_heap_end_addr();

  // Align the base to the Page size, which is a common practice.
  g_buddy.base = ALIGN(start_addr, PAGE_SIZE);
  g_buddy.size = end_addr - g_buddy.base; // Calculate the end & beginning to figure out the total available size.
```

## The math

The buddy allactor does not accept a certain amount of pages. It instead needs one variable which is the order
of the pages. If Order = 0, it would need one page, Order = 1 is two pages, and Order 2 = three pages.
You can simply calculate the order by:

$$ \text{order} = \lfloor \log_2(\frac{\texttt{total\_memory}}{\texttt{PAGE\_SIZE}}) \rfloor $$

### An example

Let's say we have an available memory of 4Mb (`4 * 1024 * 1024`). This is perfectly aligned to our Page size of 4096.
To find the order of the total amount of memory. We will use the formula I talked about earlier:

$$ \text{} = \lfloor \log_2(\frac{\texttt{4 * 1024 * 1024}}{\texttt{PAGE\_SIZE}}) \rfloor $$

The order is 10 (1024 pages).

```c
void buddy_init(void) {
  uintptr_t start_addr = (uintptr_t)get_next_free_addr();
  uintptr_t end_addr = (uintptr_t)get_heap_end_addr();

  g_buddy.base = ALIGN(start_addr, PAGE_SIZE);
  uintptr_t total_size = end_addr - g_buddy.base;

  int32_t bitmap_bytes = CEIL_DIV(total_size / PAGE_SIZE, 8);
  g_buddy.map_size = CEIL_DIV(bitmap_bytes, sizeof(size_t)) * sizeof(size_t);

  g_buddy.size = total_size - g_buddy.map_size;
  g_buddy.max_order = log2(g_buddy.size / PAGE_SIZE);

  g_buddy.map = (uint8_t *)g_buddy.base;
  memset(g_buddy.map, 0, g_buddy.map_size);

  for (int i = 0; i <= g_buddy.max_order; i++) {
    g_buddy.free_lists[i] = NULL;
  }

  size_t remaining = g_buddy.size;
  uintptr_t current_addr = ALIGN(g_buddy.base + g_buddy.map_size, PAGE_SIZE);
  while (remaining >= PAGE_SIZE) {
    uint32_t order = log2(remaining / PAGE_SIZE);
    if (order > g_buddy.max_order) {
      order = g_buddy.max_order;
    }

    size_t block_size = (1 << order) * PAGE_SIZE;
    vmm_remap_page(SCRATCH_VADDR, (void *)current_addr, 0);
    buddy_node_t *block = (buddy_node_t *)SCRATCH_VADDR;
    block->next = g_buddy.free_lists[order];
    g_buddy.free_lists[order] = (buddy_node_t *)current_addr;

    current_addr += block_size;
    remaining -= block_size;
  }
}
```
