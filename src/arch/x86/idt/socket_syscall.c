#include "arch/x86/idt/syscalls.h"
#include "drivers/printk.h"
#include "fs/stat.h"
#include "fs/vfs.h"
#include "memory/kmalloc.h"
#include "net/socket.h"
#include "sys/file/fcntl.h"
#include "sys/file/file.h"

#include <ferrite/errno.h>
#include <ferrite/types.h>
#include <lib/stdlib.h>
#include <ferrite/string.h>

SYSCALL_ATTR static s32 sys_bind(s32 fd, void* addr, s32 addrlen)
{
    file_t* f = fd_get(fd);
    if (!f || !S_ISSOCK(f->f_inode->i_mode)) {
        return -1;
    }

    socket_t* s = f->f_inode->u.i_socket;
    if (!s || !s->ops || !s->ops->bind) {
        return -1;
    }

    return s->ops->bind(s, addr, addrlen);
}

SYSCALL_ATTR static s32 sys_listen(int fd, int backlog)
{
    file_t* f = fd_get(fd);
    if (!f || !S_ISSOCK(f->f_inode->i_mode)) {
        return -1;
    }

    socket_t* s = f->f_inode->u.i_socket;
    if (!s || !s->ops || !s->ops->listen) {
        return -1;
    }

    return s->ops->listen(s, backlog);
}

SYSCALL_ATTR static s32 sys_connect(s32 fd, void* addr, size_t addrlen)
{
    file_t* f = fd_get(fd);
    if (!f || !S_ISSOCK(f->f_inode->i_mode)) {
        return -1;
    }

    socket_t* s = f->f_inode->u.i_socket;
    if (!s || !s->ops || !s->ops->bind) {
        return -1;
    }

    return s->ops->connect(s, addr, (s32)addrlen);
}

SYSCALL_ATTR static s32 sys_socket(s32 family, s16 type, s32 protocol)
{
    return socket_create(family, type, protocol);
}

static int sys_accept(int fd, void* addr, int const* addrlen)
{
    (void)addr;
    (void)addrlen;

    file_t* f = fd_get(fd);
    if (!f || !S_ISSOCK(f->f_inode->i_mode)) {
        return -EINVAL;
    }

    socket_t* s = f->f_inode->u.i_socket;
    if (!s || !s->ops || !s->ops->accept) {
        return -EIO;
    }

    socket_t* newsock = kmalloc(sizeof(socket_t));
    if (!newsock) {
        return -ENOMEM;
    }

    memset(newsock, 0, sizeof(socket_t));
    if (s->ops->accept(s, newsock) < 0) {
        kfree(newsock);
        return -1;
    }

    vfs_inode_t* inode = __alloc_socket(newsock);
    if (!inode) {
        if (newsock->data) {
            kfree(newsock->data);
        }

        kfree(newsock);

        return -1;
    }

    int new_fd = fd_alloc();
    if (new_fd < 0) {
        // Leaking?

        inode_put(inode);
        return fd;
    }

    file_t* newfile = fd_get(new_fd);
    if (!newfile) {
        abort("Something went seriously wrong");
    }

    newfile->f_inode = inode;
    newfile->f_op = f->f_op;

    newfile->f_pos = 0;
    newfile->f_mode = FMODE_READ | FMODE_WRITE;
    newfile->f_flags = O_RDWR;
    newfile->f_count = 1;

    return new_fd;
}

int sys_socketcall(int call, unsigned long* args)
{
    switch (call) {
    case SYS_SOCKET:
        return sys_socket((int)args[0], (s16)args[1], (int)args[2]);

    case SYS_BIND:
        return sys_bind((int)args[0], (void*)args[1], (int)args[2]);

    case SYS_CONNECT:
        return sys_connect((int)args[0], (void*)args[1], (int)args[2]);

    case SYS_LISTEN:
        return sys_listen((int)args[0], (int)args[1]);

    case SYS_ACCEPT:
        return sys_accept((int)args[0], (void*)args[1], (int*)args[2]);

    default:
        printk("Nothing...?\n");
        return -ENOSYS;
    }
}
