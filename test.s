.section .text
.global  _start

_start:
	mov $4, %eax
	mov $1, %ebx
	mov $msg, %ecx
	mov $13, %edx
	int $0x80

	movl $1, %eax
	movl $42, %ebx
	int  $0x80

	hlt

msg:
	.ascii "Hello World!\n"
