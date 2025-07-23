# KFS 02 - GDT & Stack

## Introduction

Let's proceed to the second `KFS` project. The first was doable and I felt confident doing the second one.

For this project, we had to implement a GDT (Global Descriptor Table). The GDT serves as a fundamental data structure in x86 architecture, playing a crucial role in memory management and protection. When our computer starts, it begins in `real mode`, a simple operating mode that provides direct access to memory and I/O devices. However we need to switch to protected mode, which introduces memory protection, virtual memory, and privilege levels.

Think of `protected mode` as establishing different security clearance levels in a building. The GDT acts as the rulebook for this security system, defining the access rights and boundaries for each level. The x86 architecture provides four rings (0-3), where ring 0 is the most privileged (kernel space) and ring 3 is the least privileged (user space). Each ring has specific permissions and restrictions, all defined in our GDT.

The GDT is essential not just for security, but also for the basic operation of `protected mode`. Without a properly configured GDT, the CPU cannot execute `protected mode` code at all.

## Goals

The project requires creating a GDT at 0x00000800 with entries for Kernel Data Space, Kernel Code Space, User Data Space, and User Code Space. Additionally, we need to add minimal PS/2 Keyboard Support and implement a basic shell with the commands `reboot` & `gdt`. The `gdt` command will print the GDT entries in a human-readable way.

## Technical Approach & Implementation

My journey began with studying the [OSDev](https://wiki.osdev.org/Global_Descriptor_Table) documentation. The concepts were initially overwhelming. The terminology they used like segment descriptors, privilege levels, and descriptor flags felt like learning a new language. After watching several YouTube tutorials ([here](https://www.youtube.com/watch?v=GvIJYELuaaE&t=5615s) & [here](https://www.youtube.com/watch?v=Wh5nPn2U_1w&t=429s)) about GDT implementation, things started to click.

I faced a choice: implement the GDT in Assembly or C. While Assembly would give more direct control, I chose C for its human-readability and my familiarity with the language. Here's how I structured the implementation:

The boot process begins in boot.asm, where we set up multiboot flags and prepare for the transition to protected mode. Once we call `kmain()`, it would call the function `gdt_init()`. This would create the GDT that we will use in our kernel.

```c
extern void gdt_flush(uint32_t);

entry_t gdt_entries[5] __attribute__((section(".gdt")));
descriptor_pointer_t gdt_ptr;


void gdt_set_gate(uint32_t num, uint32_t base, uint32_t limit, uint8_t access,
                  uint8_t gran) {
    ...
}

void gdt_init() {
  gdt_ptr.limit = sizeof(entry_t) * 5 - 1;
  gdt_ptr.base = (uint32_t)&gdt_entries;

  gdt_set_gate(0, 0, 0, 0, 0);                // NULL Gate
  gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Kernel Code Segment
  gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Kernel Data Segment
  gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User Code Segment
  gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User Data Segment

  gdt_flush((uint32_t)&gdt_ptr);
}
```

We call gdt_set_gate() which creates what are known as `segment descriptors` or `gates`. The function takes five parameters:

- num: The index of the current Gate we are configuring
- base: The starting address of the segment (`0` for flat memory model)
- limit: The maximum addressable unit (`0xFFFFFFFF` means use entire 4GB address space)
- access: Defines segment privileges and type
- granularity: Controls granularity and size

At the top of the code snippet you will see two variables.
`entry_t gdt_entries[5] __attribute__((section(".gdt")));` is where we will place the entries in. We will have 5 gates in total. The `__attribute__((section(".gdt")))` is a compiler directive that instructs the `linker` to place this array in a special data section named `.gdt`. I then position this section at a specific memory address using the linker script.

`descriptor_pointer_t gdt_ptr` is what we put into the `lgdt` register. The CPU needs this to know where it can find the `GDT Gates` & what its size is.

After setting up the GDT, I implemented basic keyboard support. While my current polling approach isn't ideal (it continuously checks for keystrokes), it works for our basic shell. A proper implementation would use interrupts to handle keyboard events, but that's a topic for future projects. The VGA driver from KFS_01 was adapted to create a simple shell interface, allowing for the `reboot` and `gdt` commands.

The system still experienced triple faults initially. The cause of the initial triple faults was the `linker script`. Although I used the `__attribute__` to place the GDT in a custom section, I hadn't yet told the `linker` where to place that `.gdt` section in the final memory layout, leading to the crash. I ensured our GDT was placed at the correct memory address. The ordering is crucial: BIOS boot code, then GDT, then the rest of our kernel.

```
  /* Start at 2MB */
  . = 2M;


  .gdt 0x800 : ALIGN(0x800)
    {
    *(.gdt)
  }

  /* The rest... */
```

By using the command: `objdump -h ferrite-c.elf | grep gdt` in your terminal. It will show something like this:

```
0 .gdt          00000028  00000800  00000800  00000800  2**11
```

The second column show `00000800`, which means you did it! The GDT is placed in 0x00000800. Alternativly, you can write in your program:

```c
void print_gdt(void) {
  descriptor_pointer_t gdtr;

  __asm__ volatile("sgdt %0" : "=m"(gdtr));

  printk("GDT Base Address: 0x%x\n", gdtr.base);
  printk("GDT Limit: 0x%x\n", gdtr.limit);
}
```

## Challenges

The challenges were mostly understanding the GDT. I struggled to grasp its purpose and exact workings. It took me reading several articles and watching multiple videos to finally understand what it's meant to do.

I also had no real experience with the linker. Finding the source of the triple fault was particularly frustrating, and it took quite a while before I realized the linker might not be placing the GDT at the correct address.

## Conclusion & Lesson Learned

I found that I needed to reread materials multiple times to fully grasp concepts. Fortunately, there was plenty of documentation available about the GDT and its implementation. Working with the GDT motivated me to document everything extensively, like these pages. I mainly do this to ensure I truly understand the functionality of each component I'm working with. And to help others who might follow this path, of course. ;)
