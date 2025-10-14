#include "arch/x86/idt/syscalls.h"
#include "drivers/printk.h"
#include "lib/string.h"
#include "memory/kmalloc.h"
#include "net/socket.h"
#include "sys/file/fcntl.h"
#include "sys/file/file.h"
#include "sys/file/inode.h"
#include "sys/file/stat.h"
#include "types.h"

SYSCALL_ATTR static s32 sys_bind(s32 fd, void* addr, s32 addrlen)
{
    file_t* f = getfd(fd);
    if (!f || S_ISSOCK(f->f_inode->i_mode)) {
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
    file_t* f = getfd(fd);
    if (!f || S_ISSOCK(f->f_inode->i_mode)) {
        return -1;
    }

    socket_t* s = f->f_inode->u.i_socket;
    if (!s || !s->ops || !s->ops->bind) {
        return -1;
    }

    return s->ops->listen(s, backlog);
}

SYSCALL_ATTR static s32 sys_connect(s32 fd, void* addr, size_t addrlen)
{
    file_t* f = getfd(fd);
    if (!f || S_ISSOCK(f->f_inode->i_mode)) {
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

    file_t* f = getfd(fd);
    if (!f || S_ISSOCK(f->f_inode->i_mode)) {
        return -1;
    }

    socket_t* s = f->f_inode->u.i_socket;
    if (!s || !s->ops || !s->ops->bind) {
        return -1;
    }

    socket_t* newsock = kmalloc(sizeof(socket_t));
    if (!newsock) {
        return -1;
    }

    memset(newsock, 0, sizeof(socket_t));
    if (s->ops->accept(s, newsock) < 0) {
        kfree(newsock);
    }

    struct inode* inode = __alloc_inode(S_IFSOCK | 0666);
    if (!inode) {
        if (newsock->data) {
            kfree(newsock->data);
        }
        kfree(newsock);
        return -1;
    }

    inode->i_count = 1;
    inode->u.i_socket = newsock;
    newsock->inode = inode;

    struct file* newfile = __alloc_file();
    if (!newfile) {
        if (newsock->data) {
            kfree(newsock->data);
        }
        kfree(newsock);
        __dealloc_inode(inode);
        return -1;
    }

    newfile->f_inode = inode;
    newfile->f_count = 1;
    newfile->f_pos = 0;
    newfile->f_flags = O_RDWR;
    newfile->f_mode = FMODE_READ | FMODE_WRITE;
    newfile->f_op = f->f_op;

    proc_t* p = myproc();
    for (int fd = 0; fd < MAX_OPEN_FILES; fd++) {
        if (!p->open_files[fd]) {
            p->open_files[fd] = newfile;
            return fd;
        }
    }

    __dealloc_inode(inode);
    __dealloc_file(f);

    if (newsock->data) {
        kfree(newsock->data);
    }
    kfree(newsock);
    return -1;
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
        return -1;
    }
}
