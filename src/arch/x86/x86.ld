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

KERNEL_VIRTUAL_BASE = 0xC0000000;
KERNEL_PHYSICAL_BASE = 0x200000;

ENTRY(_start)

SECTIONS {
  /* Start at 2MB */
	. = KERNEL_PHYSICAL_BASE;

  .gdt 0x800 : ALIGN(0x800) {
    *(.gdt)
  }

  .note.gnu.build-id ALIGN(4K) : {
    *(.note.gnu.build-id)
  }

  .boot ALIGN(4K) : {
    *(.multiboot)
    *(.boot)
  }

  . = ALIGN(4K);
  . += KERNEL_VIRTUAL_BASE;

  /* --- Higher-Half Kernel Sections --- */

  .text ALIGN(4K) : AT(ADDR(.text) - KERNEL_VIRTUAL_BASE) {
    *(.text)
  }

  .rodata ALIGN(4K) : AT(ADDR(.rodata) - KERNEL_VIRTUAL_BASE) {
    *(.rodata)
  }

  .data ALIGN(4K) : AT(ADDR(.data) - KERNEL_VIRTUAL_BASE) {
    *(.data)
  }

  .bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_VIRTUAL_BASE) {
    _bss_start = .; 
    *(COMMON)
    *(.bss)
    _bss_end = .; 
  }

  . = ALIGN(4K);
  _virtual_kernel_end = .;
  _physical_kernel_end = _virtual_kernel_end - KERNEL_VIRTUAL_BASE;
}
