	BITS 32

	section .text

	global _start
	extern main
	extern exit

_start:
	;   Clear frame pointer
	xor ebp, ebp

	; Stack layout at entry:
	; [esp+0] = argc
	; [esp+4] = argv[0]
	; [esp+8] = argv[1]
	; ...

	;   Get argc
	pop eax; eax = argc
	mov ebx, esp; ebx = argv

	;   Calculate envp (argv + argc + 1)
	lea ecx, [esp + eax*4 + 4]

	;   Align stack to 16 bytes
	and esp, 0xFFFFFFF0

	;    Push arguments in reverse order
	push ecx; envp
	push ebx; argv
	push eax; argc

	;    Call main
	call main

	;    Exit with return value
	push eax
	call exit

	; Should never reach here
	hlt
