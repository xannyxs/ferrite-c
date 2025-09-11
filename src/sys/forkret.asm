section .text
global  forkret

extern trapret

forkret:
	jmp trapret
