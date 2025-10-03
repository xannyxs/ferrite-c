section .text
global  syscall_handler

extern syscall_dispatcher_c

	; This is the entry point for `int 0x80`

syscall_handler:
	push dword 0
	push dword 0x80

	push ds
	push es
	push fs
	push gs

	push edi
	push esi
	push ebp
	push esp
	push ebx
	push edx
	push ecx
	push eax

	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	push esp
	call syscall_dispatcher_c
	add  esp, 4

	pop eax
	pop ecx
	pop edx
	pop ebx
	add esp, 4
	pop ebp
	pop esi
	pop edi

	pop gs
	pop fs
	pop es
	pop ds

	add esp, 8

	iret
