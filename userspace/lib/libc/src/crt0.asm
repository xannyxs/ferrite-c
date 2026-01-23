	BITS 32

	section .text
	global  _start
	extern  main
	extern  exit

_start:
	xor ebp, ebp

	and esp, 0xFFFFFFF0

	mov eax, [esp]; argc
	lea ebx, [esp + 4]; argv
	lea ecx, [esp + eax*4 + 8]; envp

	sub esp, 16
	mov [esp+8], ecx; envp
	mov [esp+4], ebx; argv
	mov [esp], eax; argc

	call main

	push eax
	call exit
	hlt
