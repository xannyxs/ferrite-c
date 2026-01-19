// userspace/lib/libc/syscalls.h
#ifndef SYSCALLS_H
#define SYSCALLS_H

typedef unsigned int size_t;
typedef int pid_t;
typedef int ssize_t;

// Process
void exit(int status) __attribute__((noreturn));
void _exit(int status) __attribute__((noreturn));
pid_t fork(void);
int execve(char const* path, char* const argv[], char* const envp[]);
pid_t waitpid(int* status);
pid_t getpid(void);

// I/O
ssize_t read(int fd, void* buf, size_t count);
ssize_t write(int fd, void const* buf, size_t count);

// String
#define memcpy(dest, src, n) __builtin_memcpy((dest), (src), (n))
#define memset(s, c, n) __builtin_memset((s), (c), (n))

int strlen(char const* s);
int strcmp(char const* a, char const* b);
int strncmp(char const* a, char const* b, size_t n);

#endif
