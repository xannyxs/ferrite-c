#ifndef _LIBC_SYSCALLS_H
#define _LIBC_SYSCALLS_H

#include <uapi/dirent.h>
#include <uapi/stat.h>
#include <uapi/types.h>

typedef int pid_t;
typedef int ssize_t;

void exit(int status) __attribute__((noreturn));
void _exit(int status) __attribute__((noreturn));

pid_t fork(void);
int execve(char const* path, char* const argv[], char* const envp[]);
pid_t waitpid(int* status);
pid_t getpid(void);

ssize_t read(int fd, void* buf, size_t count);
ssize_t write(int fd, void const* buf, size_t count);
int close(int fd);
char* getcwd(char* buf, size_t size);
int readdir(unsigned int fd, dirent_t* dirp, unsigned int count);
int stat(char const* pathname, struct stat* statbuf);
int open(char const*, int, int);
int chdir(char const*);

#endif
