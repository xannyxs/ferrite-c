#include "fs/stat.h"
#include "fs/vfs.h"
#include "idt/syscalls.h"
#include "sys/process/process.h"

#include <ferrite/errno.h>

SYSCALL_ATTR int sys_truncate(char const* path, off_t len)
{
    vfs_inode_t* node = vfs_lookup(myproc()->root, path);
    if (!node) {
        return -ENOENT;
    }

    if (S_ISDIR(node->i_mode) || !vfs_permission(node, MAY_WRITE)) {
        inode_put(node);
        return -EACCES;
    }

    // if (IS_RDONLY(node)) {
    //     inode_put(node);
    //     return -EROFS;
    // }

    if (!node->i_op->truncate) {
        inode_put(node);
        return -ENOSYS;
    }

    int ret = node->i_op->truncate(node, len);

    inode_put(node);
    return ret;
}

SYSCALL_ATTR int sys_ftruncate(int fd, off_t len)
{
    file_t* file = fd_get(fd);
    if (!file) {
        return -EBADF;
    }

    if (!file->f_inode->i_op->truncate) {
        return -ENOSYS;
    }

    int ret = file->f_inode->i_op->truncate(file->f_inode, len);
    return ret;
}
