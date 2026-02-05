section .text

global syscall_handler
global trapret
extern syscall_dispatcher_c

syscall_handler:
	push dword 0; err_code
	push dword 0x80; int_no

	push ds
	push es
	push fs
	push gs

	pusha

	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	push esp; Pass trapframe pointer
	call syscall_dispatcher_c
	add  esp, 4

trapret:
	popa
	pop gs
	pop fs
	pop es
	pop ds
	add esp, 8; Skip int_no and err_code
	iret
