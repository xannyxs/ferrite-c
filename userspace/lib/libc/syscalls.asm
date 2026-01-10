	; userspace/lib/libc/syscalls.asm
	; System call wrappers for userspace

	BITS 32

	;       Syscall numbers (must match your kernel!)
	%define SYS_EXIT     1
	%define SYS_FORK     2
	%define SYS_READ     3
	%define SYS_WRITE    4
	%define SYS_OPEN     5
	%define SYS_CLOSE    6
	%define SYS_WAIT     7
	%define SYS_EXECVE   11
	%define SYS_CHDIR    12
	%define SYS_GETPID   20

	section .text

	;      void exit(int status)
	global exit
	global _exit

exit:
_exit:
	mov eax, SYS_EXIT
	mov ebx, [esp+4]
	int 0x80
	hlt

	;      int write(int fd, const void* buf, size_t count)
	global write

write:
	push ebx
	mov  eax, SYS_WRITE
	mov  ebx, [esp+8]; fd
	mov  ecx, [esp+12]; buf
	mov  edx, [esp+16]; count
	int  0x80
	pop  ebx
	ret

	;      int read(int fd, void* buf, size_t count)
	global read

read:
	push ebx
	mov  eax, SYS_READ
	mov  ebx, [esp+8]; fd
	mov  ecx, [esp+12]; buf
	mov  edx, [esp+16]; count
	int  0x80
	pop  ebx
	ret

	;      pid_t fork(void)
	global fork

fork:
	mov eax, SYS_FORK
	int 0x80
	ret

	;      int execve(const char* path, char* const argv[], char* const envp[])
	global execve

execve:
	push ebx
	push esi
	mov  eax, SYS_EXECVE
	mov  ebx, [esp+12]; path
	mov  ecx, [esp+16]; argv
	mov  edx, [esp+20]; envp
	int  0x80
	pop  esi
	pop  ebx
	ret

	;      pid_t wait(int* status)
	global waitpid

waitpid:
	push ebx
	mov  eax, SYS_WAIT
	mov  ebx, [esp+8]
	int  0x80
	pop  ebx
	ret

	;      pid_t getpid(void)
	global getpid

getpid:
	mov eax, SYS_GETPID
	int 0x80
	ret
