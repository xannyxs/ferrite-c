section .text
global  swtch

	; void swtch(struct context **old, struct context *new)

swtch:
	mov eax, [esp + 4]
	mov edx, [esp + 8]

	push ebp
	push ebx
	push esi
	push edi

	mov [eax], esp
	mov esp, edx

	pop ebp
	pop ebx
	pop esi
	pop edi

	ret
