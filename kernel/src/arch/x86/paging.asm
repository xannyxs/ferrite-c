	BITS 32

	section .text
	global  enable_paging
	global  load_page_directory
	global  flush_tlb

	; The TLB is not transparently informed of changes made to paging structures.
	; Therefore the TLB has to be flushed upon such a change. On x86 systems
	; this can be done by writing to the page directory base register (CR3):

flush_tlb:
	mov eax, cr3
	mov cr3, eax

	ret

enable_paging:
	push ebp
	mov  ebp, esp

	;   Enable paging
	mov eax, cr0; Read the Control Register 0
	or  eax, 0x80000000; Set the PG (Paging) bit
	mov cr0, eax; Write the new value back to CR0

	mov esp, ebp
	pop ebp
	ret

load_page_directory:
	push ebp
	mov  ebp, esp

	;   Load the address of the page directory table
	mov eax, [esp + 8]; Get the first argument from the stack

	mov cr3, eax; Load the address into Control Register 3

	mov esp, ebp
	pop ebp
	ret
