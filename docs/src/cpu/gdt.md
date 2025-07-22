# Global Descriptor Table (GDT)

## What is it

The GDT serves as a fundamental data structure in x86 architecture, playing a crucial role in memory management and protection. When our computer starts, it begins in `real mode`, a simple operating mode that provides direct access to memory and I/O devices. However we need to switch to protected mode, which introduces memory protection, virtual memory, and privilege levels.

Think of `protected mode` as establishing different security clearance levels in a building. The GDT acts as the rulebook for this security system, defining the access rights and boundaries for each level. The x86 architecture provides four rings (0-3), where ring 0 is the most privileged (kernel space) and ring 3 is the least privileged (user space). Each ring has specific permissions and restrictions, all defined in our GDT.

The GDT is essential not just for security, but also for the basic operation of `protected mode`. Without a properly configured GDT, the CPU cannot execute `protected mode` code at all.

For more information go to [OSDev](https://wiki.osdev.org/Global_Descriptor_Table)

## My Technical Approach

My approach was as follows: I would start at the `boot.asm` & setup the multiboot. After everything is setup, I would call `kmain()`, which then calls `gdt_init()`. `gdt_init()` will setup the `segment descriptors` of the GDT & ensure that it creates the correct struct pointer that will be passed to `gdt_flush`. `gdt_flush` will place the entries in the correct registers for the CPU to read.

Here are some snippets to give you a better idea:

```c
extern void gdt_flush(uint32_t); // The Assembly function

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

The assembly code in `gdt_flush.asm` that actually loads the GDT:

```nasm
    gdt_flush:
        ; Get the address of the variable we passed to the function
        mov  eax, [esp + 4]
        lgdt [eax]

        ;   Set up segments
        mov eax, 0x10
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax

        ;   Far jump to flush pipeline and load CS
        jmp 0x08:.flush

    .flush:
        ret
```

## Considerations

I considered doing a full assembly setup, but the reason I did not is simply because the C language ensures the readibilty.

## Linker

I force the `Linker` to put GDT before the rest of the code. You can do it without this, but you will have issues if you would like to place it in a specific address. If that is not necessary for you, you can ignore this. The specific placement of the GDT in memory can be important for some system designs, particularly when dealing with memory management and virtual memory setup. This was my approach:

```
  /* Start at 2MB */
  . = 2M;


  .gdt 0x800 : ALIGN(0x800)
    {
    *(.gdt)
  }
```

### Memory Layout Considerations

When placing the GDT at a specific address, it's important to ensure that:

1. The address is accessible during the transition to protected mode
2. The address doesn't conflict with other important system structures
3. The address is properly aligned for optimal performance
