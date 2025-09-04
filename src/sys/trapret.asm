section .text
global  trapret

	; trapret (trap return) is the low-level assembly
	; routine that handles the actual transition
	; from kernel mode back to user mode.

trapret:
	popal
	pop gs
	pop fs
	pop es
	pop ds
	add esp, 0x8; trapno and errcode
	iret
