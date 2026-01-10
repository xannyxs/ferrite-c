section .text

extern exception_dispatcher_c

global divide_by_zero_stub
global debug_interrupt_stub
global non_maskable_interrupt_stub
global breakpoint_stub
global overflow_stub
global bound_range_exceeded_stub
global invalid_opcode_stub
global device_not_available_stub
global double_fault_stub
global invalid_tss_stub
global segment_not_present_stub
global stack_segment_fault_stub
global general_protection_fault_stub
global page_fault_stub
global x87_fpu_exception_stub
global reserved

common_exception_handler:
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
	call exception_dispatcher_c
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

divide_by_zero_stub:
	push dword 0xDEADBEEF
	push dword 0
	jmp  common_exception_handler

debug_interrupt_stub:
	push dword 0xDEADBEEF
	push dword 1
	jmp  common_exception_handler

non_maskable_interrupt_stub:
	push dword 0
	push dword 2
	jmp  common_exception_handler

breakpoint_stub:
	push dword 0
	push dword 3
	jmp  common_exception_handler

overflow_stub:
	push dword 0
	push dword 4
	jmp  common_exception_handler

bound_range_exceeded_stub:
	push dword 0
	push dword 5
	jmp  common_exception_handler

invalid_opcode_stub:
	push dword 0
	push dword 6
	jmp  common_exception_handler

device_not_available_stub:
	push dword 0
	push dword 7
	jmp  common_exception_handler

double_fault_stub:
	push dword 8
	jmp  common_exception_handler

invalid_tss_stub:
	push dword 10
	jmp  common_exception_handler

segment_not_present_stub:
	push dword 11
	jmp  common_exception_handler

stack_segment_fault_stub:
	push dword 12
	jmp  common_exception_handler

general_protection_fault_stub:
	push dword 13
	jmp  common_exception_handler

page_fault_stub:
	push dword 14
	jmp  common_exception_handler

x87_fpu_exception_stub:
	push dword 15
	jmp  common_exception_handler

reserved:
	ret
