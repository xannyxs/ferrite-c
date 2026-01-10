section .text

extern irq_dispatcher_c
global irq_stub_0
global irq_stub_1
global irq_stub_7

common_irq_handler:
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
	call irq_dispatcher_c
	add  esp, 4

	pop eax
	pop ecx
	pop edx
	pop ebx
	pop esp
	pop ebp
	pop esi
	pop edi

	pop gs
	pop fs
	pop es
	pop ds

	add esp, 8
	iret

irq_stub_0:
	push dword 0
	push dword 0x20
	jmp  common_irq_handler

irq_stub_1:
	push dword 0
	push dword 0x21
	jmp  common_irq_handler

irq_stub_7:
	push dword 0
	push dword 0x27
	jmp  common_irq_handler

