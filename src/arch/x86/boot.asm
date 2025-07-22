	;------------------------------------------------------------------------------
	; Boot Assembly Entry Point

	; This file serves as the entry point for our kernel, setting up the minimal
	; environment needed before we can jump to our Rust code. It performs three
	; critical tasks:

	; 1. Creates a multiboot header that marks this as a kernel for GRUB/multiboot
	; bootloaders. The header includes magic numbers and flags that tell the
	; bootloader how to load us.
	; 2. Sets up a small stack (16KB) that our kernel will initially use. The stack
	; is crucial because Rust code requires it for function calls and local variables.

	; 3. Transfers control from assembly to our main kernel code written in Rust
	; ensuring we never return from it (since there's nothing to return to).
	;------------------------------------------------------------------------------

	;        Multiboot header constants
	MBALIGN  equ 1<<0; Align loaded modules on page boundaries
	MEMINFO  equ 1<<1; Provide memory map
	MBFLAGS  equ MBALIGN | MEMINFO; Combine our flags
	MAGIC    equ 0x1BADB002; Magic number lets bootloader find the header
	CHECKSUM equ -(MAGIC + MBFLAGS); Checksum required by multiboot standard

	;       First section: Multiboot header
	section .multiboot
	align   4; Header must be 4-byte aligned
	dd      MAGIC; Write the magic number
	dd      MBFLAGS; Write the flags
	dd      CHECKSUM; Write the checksum

	; ----------------------------------------------

	;       Second section: Stack setup
	section .bss

	align 16; Ensure proper alignment for the stack

stack_bottom:
	resb 16384; Reserve 16KB for our stack

stack_top:

	; ----------------------------------------------

	section .data

	; ----------------------------------------------

	section .text
	global  _start

_start:
	mov ecx, (initial_page_dir - 0xC0000000)
	mov cr3, ecx

	mov ecx, cr4
	or  ecx, 0x10
	mov cr4, ecx

	mov ecx, cr0
	or  ecx, 0x80000000
	mov cr0, ecx

	jmp higher_half

higher_half:
	mov esp, stack_top

	;    Add magic number and mutliboot header into stack
	push ebx
	push eax

	cli

	;      Call kernel
	extern kmain
	call   kmain

.hang:
	cli
	hlt ; Halt the CPU
	jmp .hang

section .data
align   4096
global  initial_page_dir

initial_page_dir:
	dd    10000011b
	times (768 - 1) dd 0

	dd    (0 << 22) | 10000011b
	dd    (1 << 22) | 10000011b
	dd    (2 << 22) | 10000011b
	dd    (3 << 22) | 10000011b
	times (1024 - 768 - 4) dd 0
