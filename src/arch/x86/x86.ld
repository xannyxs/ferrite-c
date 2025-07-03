/* This linker script organizes the memory layout for an x86 kernel.
* It places the program at the 2MB mark in memory and arranges different
* types of data into separate 4KB-aligned sections:
* 
* - .text:   Contains executable code and multiboot header
* - .rodata: Holds read-only data like constants
* - .data:   Stores initialized variables
* - .bss:    Reserved for uninitialized variables
* 
* The 4KB alignment matches the x86 page size, enabling proper memory
* protection and efficient virtual memory management.
*/

ENTRY(_start)

SECTIONS {
  .gdt 0x800 : ALIGN(0x800) {
    *(.gdt)
  }

  . = 0xC0100000;

  /* Text section with multiboot header */
  .text : AT(0x100000) {
      *(.multiboot)
      *(.text)
  }

  /* Read-only data. */
  .rodata : {
    *(.rodata)
  }

  . = ALIGN(4K);

  /* Read-write data (initialized) */
  .data : {
    *(.data)
  }

  /* Read-write data (uninitialized) and stack */
  .bss : {
    *(COMMON)
    *(.bss)
  }

  . = ALIGN(4K);
  pmm_bitmap = .;

  _kernel_end = .;
}
