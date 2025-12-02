#include "arch/x86/idt/syscalls.h"
#include "arch/x86/idt/idt.h"
#include "arch/x86/time/time.h"
#include "drivers/printk.h"
#include "ferrite/dirent.h"
#include "fs/ext2/ext2.h"
#include "fs/stat.h"
#include "fs/vfs.h"
#include "sys/file/fcntl.h"
#include "sys/file/file.h"
#include "sys/process/process.h"
#include "sys/signal/signal.h"
#include "sys/timer/timer.h"
#include "syscalls.h"

#include <ferrite/errno.h>
#include <ferrite/types.h>
#include <lib/string.h>

extern vfs_inode_t* root_inode;

__attribute__((target("general-regs-only"))) static void sys_exit(s32 status)
{
    do_exit(status);
}

SYSCALL_ATTR static s32 sys_read(s32 fd, void* buf, int count)
{
    file_t* f = fd_get(fd);
    if (!f || !f->f_inode) {
        return -1;
    }

    if (f->f_op && f->f_op->read) {
        return f->f_op->read(f->f_inode, f, buf, count);
    }

    return -1;
}

SYSCALL_ATTR static s32 sys_write(s32 fd, void* buf, int count)
{
    file_t* f = fd_get(fd);
    if (!f || !f->f_inode) {
        return -1;
    }

    if (f->f_op && f->f_op->write) {
        return f->f_op->write(f->f_inode, f, buf, count);
    }

    return -1;
}

// TODO: Support flags:
// https://man7.org/linux/man-pages/man2/open.2.html
SYSCALL_ATTR static s32 sys_open(char const* path, int flags, int mode)
{
    vfs_inode_t* root = inode_get(root_inode->i_sb, 2);
    if (!root) {
        return -1;
    }

    vfs_inode_t* new = vfs_lookup(root, path);
    if (!new) {
        if (!(flags & O_CREAT)) {
            return -ENOENT;
        }

        char* last_slash = strrchr(path, '/');
        if (!last_slash) {
            return -ENOENT;
        }

        size_t parent_len = last_slash - path;
        if (parent_len == 0) {
            parent_len = 1;
        }

        char parent_path[parent_len + 1];
        memcpy(parent_path, path, parent_len);
        parent_path[parent_len] = '\0';

        char* name = last_slash + 1;
        size_t name_len = strlen(name);

        vfs_inode_t* parent = vfs_lookup(root, parent_path);
        if (!parent) {
            return -ENOENT;
        }

        if (parent->i_op->create(
                parent, name, (s32)name_len, S_IFREG | (mode & 0777), &new
            )
            < 0) {
            inode_put(parent);
            return -1;
        }
    }

    int fd = fd_alloc();
    if (fd < 0) {
        inode_put(new);
        return -1;
    }

    file_t* file = fd_get(fd);
    if (!file) {
        file_put(myproc()->open_files[fd]);
        inode_put(new);
        return -1;
    }

    if (fd_install(new, file) < 0) {
        file_put(file);
        inode_put(new);
        return -1;
    }

    file->f_op = new->i_op->default_file_ops;
    file->f_inode = new;
    file->f_pos = 0;
    file->f_flags = flags;
    file->f_mode = mode;

    return fd;
}

SYSCALL_ATTR static int sys_close(int fd)
{
    file_t* f = fd_get(fd);
    if (!f) {
        return -1;
    }

    myproc()->open_files[fd] = NULL;

    if (f->f_op && f->f_op->release) {
        f->f_op->release(f->f_inode, f);
    }

    file_put(f);
    return 0;
}

SYSCALL_ATTR static s32 sys_fork(void) { return do_fork("user process"); }

SYSCALL_ATTR static pid_t sys_waitpid(pid_t pid, s32* status, s32 options)
{
    (void)pid;
    (void)options;
    return do_wait(status);
}

SYSCALL_ATTR static int sys_unlink(char const* path)
{
    vfs_inode_t* root = inode_get(root_inode->i_sb, 2);
    if (!root) {
        return -ENOTDIR;
    }

    if (!root->i_op || !root->i_op->unlink) {
        inode_put(root);
        return -EPERM;
    }

    char* last_slash = strrchr(path, '/');
    if (!last_slash) {
        return -1;
    }

    size_t parent_len = last_slash - path;
    if (parent_len == 0) {
        parent_len = 1;
    }

    char parent_path[parent_len + 1];
    memcpy(parent_path, path, parent_len);
    parent_path[parent_len] = '\0';

    char* name = last_slash + 1;
    size_t name_len = strlen(name);

    vfs_inode_t* parent = vfs_lookup(root, parent_path);
    if (!parent) {
        return -ENOTDIR;
    }

    int error = root->i_op->unlink(parent, name, name_len);

    inode_put(parent);
    return error;
}

SYSCALL_ATTR static time_t sys_time(time_t* tloc)
{
    time_t current_time = getepoch();

    if (tloc != NULL) {
        *tloc = current_time;
    }

    return current_time;
}

SYSCALL_ATTR int sys_stat(char const* filename, struct stat* statbuf)
{
    vfs_inode_t* root = inode_get(root_inode->i_sb, 2);
    if (!root) {
        return -ENOTDIR;
    }

    vfs_inode_t* node = vfs_lookup(root, filename);
    if (!node) {
        inode_put(root);
        return -ENOENT;
    }

    struct stat tmp;

    tmp.st_dev = node->i_dev;
    tmp.st_ino = node->i_ino;
    tmp.st_mode = node->i_mode;
    tmp.st_nlink = node->i_links_count;
    tmp.st_uid = node->i_uid;
    tmp.st_gid = node->i_gid;
    tmp.st_rdev = node->i_dev;
    tmp.st_size = node->i_size;
    tmp.st_atime = node->i_atime;
    tmp.st_mtime = node->i_mtime;
    tmp.st_ctime = node->i_ctime;
    tmp.st_blksize = node->i_sb->s_blocksize;
    tmp.st_blocks = node->u.i_ext2->i_blocks;
    memcpy(statbuf, &tmp, sizeof(tmp));

    return 0;
}

// https://man7.org/linux/man-pages/man2/lseek.2.html
SYSCALL_ATTR int sys_lseek(u32 fd, off_t offset, u32 whence)
{
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

    file_t* file = fd_get((s32)fd);
    if (!file) {
        return -EBADF;
    }

    if (file->f_op && file->f_op->lseek) {
        return file->f_op->lseek(file->f_inode, file, offset, (s32)whence);
    }

    switch (whence) {
    case SEEK_SET:
        if (offset < 0) {
            return -EINVAL;
        }

        file->f_pos = offset;
        break;

    case SEEK_CUR:
        if (file->f_pos + offset < 0) {
            return -EINVAL;
        }

        file->f_pos += offset;
        break;

    case SEEK_END:
        file->f_pos = file->f_inode->i_size + offset;
        break;

    default:
        return -EINVAL;
    }

    return file->f_pos;
}

SYSCALL_ATTR static pid_t sys_getpid(void) { return myproc()->pid; }

SYSCALL_ATTR static uid_t sys_getuid(void) { return getuid(); }

SYSCALL_ATTR static s32 sys_kill(pid_t pid, s32 sig)
{
    proc_t* caller = myproc();
    proc_t* target = find_process(pid);
    if (!target || !caller) {
        return -1;
    }

    if (caller->euid == ROOT_UID) {
        return do_kill(pid, sig);
    }

    if (caller->euid == target->euid || caller->uid == target->uid) {
        return do_kill(pid, sig);
    }

    return -1;
}

SYSCALL_ATTR static s32 sys_mkdir(char const* pathname, int mode)
{
    vfs_inode_t* node = inode_get(root_inode->i_sb, 2);
    if (!node) {
        return -EIO;
    }

    char* last_slash = strrchr(pathname, '/');
    if (!last_slash) {
        return -ENOENT;
    }

    size_t parent_len = last_slash - pathname;
    if (parent_len == 0) {
        parent_len = 1;
    }

    char parent_path[parent_len + 1];
    memcpy(parent_path, pathname, parent_len);
    parent_path[parent_len] = '\0';

    char* name = last_slash + 1;
    size_t name_len = strlen(name);

    vfs_inode_t* parent = vfs_lookup(node, parent_path);
    if (!parent) {
        return -ENOENT;
    }

    if (!parent->i_op || !parent->i_op->mkdir) {
        inode_put(node);
        inode_put(parent);
        return -ENOTDIR;
    }

    int result = parent->i_op->mkdir(
        parent, name, (s32)name_len, S_IFDIR | (mode & 0777)
    );

    inode_put(node);
    return result;
}

SYSCALL_ATTR static int sys_rmdir(char const* path)
{
    vfs_inode_t* root = inode_get(root_inode->i_sb, 2);
    if (!root) {
        return -ENOTDIR;
    }

    char const* last_slash = strrchr(path, '/');
    if (!last_slash) {
        return -ENOENT;
    }

    size_t parent_len = last_slash - path;
    if (parent_len == 0) {
        parent_len = 1;
    }

    char parent_path[parent_len + 1];
    memcpy(parent_path, path, parent_len);
    parent_path[parent_len] = '\0';

    char const* name = last_slash + 1;
    size_t name_len = strlen(name);

    vfs_inode_t* parent = vfs_lookup(root, parent_path);
    if (!parent) {
        return -ENOTDIR;
    }

    if (!parent || !parent->i_op || !parent->i_op->rmdir) {
        inode_put(parent);
        return -ENOTDIR;
    }

    return parent->i_op->rmdir(parent, name, (int)name_len);
}

SYSCALL_ATTR static uid_t sys_geteuid(void) { return geteuid(); }

SYSCALL_ATTR static s32 sys_readdir(u32 fd, dirent_t* dirent, s32 count)
{
    file_t* f = fd_get((s32)fd);
    if (!f) {
        return -1;
    }

    s32 result = -1;

    if (f->f_op && f->f_op->readdir) {
        result = f->f_op->readdir(f->f_inode, f, dirent, count);
    }

    return result;
}

SYSCALL_ATTR static s32 sys_setuid(uid_t uid)
{
    proc_t* p = myproc();

    if (p->euid == 0) {
        p->uid = uid;
        p->euid = uid;

        return 0;
    }

    if (p->uid == uid || p->euid == uid) {
        p->euid = uid;

        return 0;
    }

    return -1;
}

SYSCALL_ATTR static s32 sys_nanosleep(void) { return knanosleep(1000); }

__attribute__((target("general-regs-only"))) void
syscall_dispatcher_c(registers_t* reg)
{
    switch (reg->eax) {
    case SYS_EXIT:
        sys_exit((s32)reg->ebx);
        break;

    case SYS_FORK:
        reg->eax = sys_fork();
        break;

    case SYS_READ:
        reg->eax = sys_read((s32)reg->ebx, (void*)reg->ecx, (s32)reg->edx);
        break;

    case SYS_WRITE:
        reg->eax = sys_write((s32)reg->ebx, (void*)reg->ecx, (s32)reg->edx);
        break;

    case SYS_OPEN:
        reg->eax = sys_open((char*)reg->ebx, (s32)reg->ecx, (s32)reg->edx);
        break;

    case SYS_CLOSE:
        reg->eax = sys_close((s32)reg->ebx);
        break;

    case SYS_WAITPID:
        reg->eax = sys_waitpid((s32)reg->ebx, (s32*)reg->ecx, (s32)reg->edx);
        break;

    case SYS_UNLINK:
        reg->eax = sys_unlink((char*)reg->ebx);
        break;

    case SYS_TIME:
        reg->eax = sys_time((time_t*)reg->ebx);
        break;

    case SYS_STAT:
        reg->eax = sys_stat((char*)reg->ebx, (struct stat*)reg->ecx);
        break;

    case SYS_LSEEK:
        reg->eax = sys_lseek(reg->ebx, reg->ecx, reg->edx);
        break;

    case SYS_GETPID:
        reg->eax = sys_getpid();
        break;

    case SYS_GETUID:
        reg->eax = sys_getuid();
        break;

    case SYS_KILL:
        reg->eax = sys_kill((s32)reg->ebx, (s32)reg->ecx);
        break;

    case SYS_MKDIR:
        reg->eax = sys_mkdir((char*)reg->ebx, (s32)reg->ecx);
        break;

    case SYS_RMDIR:
        reg->eax = sys_rmdir((char*)reg->ebx);
        break;

    case SYS_SOCKETCALL:
        reg->eax = sys_socketcall((s32)reg->ebx, (unsigned long*)reg->ecx);
        break;

    case SYS_SIGNAL:
        break;

    case SYS_GETEUID:
        reg->eax = sys_geteuid();
        break;

    case SYS_READDIR:
        reg->eax = sys_readdir(reg->ebx, (void*)reg->ecx, (s32)reg->edx);
        break;

    case SYS_SETUID:
        reg->eax = sys_setuid((s32)reg->ebx);
        break;

    case SYS_NANOSLEEP:
        reg->eax = sys_nanosleep();
        break;

    default:
        printk("Nothing...?\n");
        break;
    }

    check_resched();
}
