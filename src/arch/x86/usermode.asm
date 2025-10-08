section .text

global jump_to_usermode

	; void jump_to_usermode(void* entry, void* user_stack)

jump_to_usermode:
	cli

	mov eax, [esp + 4]
	mov ecx, [esp + 8]

	mov dx, 0x23
	mov ds, dx
	mov es, dx
	mov fs, dx
	mov gs, dx

	push 0x23
	push ecx

	pushfd
	pop  edx
	or   edx, 0x200
	push edx

	push 0x1B
	push eax

	iretd
