section .text
global  syscall_handler

	; This is the entry point for `int 0x80`

syscall_handler:
	push dword 0x80
	push dword 0

	push eax
	push ecx
	push edx
	push ebx

	push esp

	push ebp
	push esi
	push edi

	push ds

	extern syscall_dispatcher_c
	push   esp; Push the `registers_t frame` as the argument
	call   syscall_dispatcher_c
	add    esp, 4; Clean up the argument `registers_t`

	pop ds

	pop edi
	pop esi
	pop ebp
	pop esp
	pop ebx
	pop edx
	pop ecx

	;    Handle EAX: Swap return value in EAX with original EAX on stack, then pop original.
	xchg eax, [esp]; Swap current EAX (C function's return) with old EAX (syscall_num) on stack
	pop  eax; Pop the original EAX (now containing the return value) into EAX

	add esp, 8

	iret
