	BITS 32

	%define SYS_EXIT     1
	%define SYS_FORK     2
	%define SYS_READ     3
	%define SYS_WRITE    4
	%define SYS_OPEN     5
	%define SYS_CLOSE    6
	%define SYS_WAIT     7
	%define SYS_UNLINK   10
	%define SYS_EXECVE   11
	%define SYS_CHDIR    12
	%define SYS_TIME     13
	%define SYS_STAT     18
	%define SYS_LSEEK    19
	%define SYS_GETPID   20
	%define SYS_MOUNT    22
	%define SYS_FSTAT    28
	%define SYS_MKDIR    39
	%define SYS_RMDIR    40
	%define SYS_BRK      45
	%define SYS_REBOOT   88
	%define SYS_READDIR  89
	%define SYS_INIT_MODULE  128
	%define SYS_DELETE_MODULE  129
	%define SYS_GETCWD   183

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

global close

close:
	push ebx
	mov  eax, SYS_CLOSE
	mov  ebx, [esp+8]
	int  0x80
	pop  ebx
	ret

global readdir

readdir:
	push ebx
	mov  eax, SYS_READDIR
	mov  ebx, [esp+8]
	mov  ecx, [esp+12]
	mov  edx, [esp+16]
	int  0x80
	pop  ebx
	ret

global stat

stat:
	push ebx
	mov  eax, SYS_STAT
	mov  ebx, [esp+8]
	mov  ecx, [esp+12]
	int  0x80
	pop  ebx
	ret

global open

open:
	push ebx
	mov  eax, SYS_OPEN
	mov  ebx, [esp+8]
	mov  ecx, [esp+12]
	mov  edx, [esp+16]
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

global getcwd

getcwd:
	push ebx
	mov  eax, SYS_GETCWD
	mov  ebx, [esp+8]
	mov  ecx, [esp+12]
	int  0x80
	pop  ebx
	ret

global chdir

chdir:
	push ebx
	mov  eax, SYS_CHDIR
	mov  ebx, [esp+8]
	int  0x80
	pop  ebx
	ret

global mkdir

mkdir:
	push ebx
	mov  eax, SYS_MKDIR
	mov  ebx, [esp+8]
	mov  ecx, [esp+12]
	int  0x80
	pop  ebx
	ret

global rmdir

rmdir:
	push ebx
	mov  eax, SYS_RMDIR
	mov  ebx, [esp+8]
	int  0x80
	pop  ebx
	ret

global unlink

unlink:
	push ebx
	mov  eax, SYS_UNLINK
	mov  ebx, [esp+8]
	int  0x80
	pop  ebx
	ret

global lseek

lseek:
	push ebx
	mov  eax, SYS_LSEEK
	mov  ebx, [esp+8]
	mov  ecx, [esp+12]
	mov  edx, [esp+16]
	int  0x80
	pop  ebx
	ret

global time

time:
	mov eax, SYS_TIME
	mov ebx, [esp+4]
	int 0x80
	ret

global reboot

reboot:
	mov eax, SYS_REBOOT
	mov ebx, [esp+4]
	mov ecx, [esp+8]
	mov edx, [esp+12]
	mov esi, [esp+16]
	int 0x80
	ret

global init_module

init_module:
	mov eax, SYS_INIT_MODULE
	mov ebx, [esp+4]
	mov ecx, [esp+8]
	mov edx, [esp+12]
	int 0x80
	ret

global delete_module

delete_module:
	mov eax, SYS_DELETE_MODULE
	mov ebx, [esp+4]
	mov ecx, [esp+8]
	int 0x80
	ret

global mount

mount:
	mov eax, SYS_MOUNT
	mov ebx, [esp+4]
	mov ecx, [esp+8]
	mov edx, [esp+12]
	mov esi, [esp+16]
	mov edi, [esp+20]

	int 0x80
	ret

global fstat

fstat:
	mov eax, SYS_FSTAT
	mov ebx, [esp+4]
	mov ecx, [esp+8]

	int 0x80
	ret

global brk

brk:
	mov eax, SYS_BRK
	mov ebx, [esp+4]

	int 0x80
	ret
