#include "fs/ext2/ext2.h"
#include "fs/mount.h"
#include "fs/stat.h"
#include "fs/vfs.h"
#include "idt/syscalls.h"
#include "memory/kmalloc.h"
#include "sys/file/fcntl.h"
#include "sys/process/process.h"

#include <ferrite/errno.h>
#include <ferrite/string.h>

SYSCALL_ATTR int sys_read(int fd, void* buf, int count)
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

SYSCALL_ATTR int sys_write(int fd, void* buf, int count)
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
SYSCALL_ATTR int sys_open(char const* path, int flags, int mode)
{
    vfs_inode_t* node = vfs_lookup(myproc()->root, path);
    if (!node) {
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

        vfs_inode_t* parent = vfs_lookup(myproc()->root, parent_path);
        if (!parent) {
            return -ENOENT;
        }

        if (!vfs_permission(parent, MAY_WRITE | MAY_EXEC)) {
            inode_put(parent);
            return -EPERM;
        }

        int ret = parent->i_op->create(
            parent, name, (int)name_len, S_IFREG | (mode & 0777), &node
        );

        inode_put(parent);

        if (ret < 0) {
            return ret;
        }
    } else {
        if ((flags & O_CREAT) && (flags & O_EXCL)) {
            inode_put(node);
            return -EEXIST;
        }

        int mask = 0;
        if ((flags & O_ACCMODE) == O_RDONLY || (flags & O_ACCMODE) == O_RDWR) {
            mask |= MAY_READ;
        }
        if ((flags & O_ACCMODE) == O_WRONLY || (flags & O_ACCMODE) == O_RDWR) {
            mask |= MAY_WRITE;
        }

        if (!vfs_permission(node, mask)) {
            inode_put(node);
            return -EPERM;
        }

        if ((flags & O_TRUNC) && (flags & (O_WRONLY | O_RDWR))) {
            if (node->i_op->truncate) {
                int ret = node->i_op->truncate(node, 0);
                if (ret < 0) {
                    inode_put(node);
                    return ret;
                }
            }
        }
    }

    int fd = fd_alloc();
    if (fd < 0) {
        inode_put(node);
        return -EMFILE;
    }

    file_t* file = fd_get(fd);
    if (!file) {
        inode_put(node);
        return -EBADF;
    }

    file->f_inode = node;
    file->f_op = node->i_op->default_file_ops;
    file->f_pos = 0;
    file->f_flags = flags;
    file->f_mode = 0;
    file->f_inode = node;
    file->f_count = 1;

    if ((flags & O_ACCMODE) == O_RDONLY || (flags & O_ACCMODE) == O_RDWR) {
        file->f_mode |= FMODE_READ;
    }

    if ((flags & O_ACCMODE) == O_WRONLY || (flags & O_ACCMODE) == O_RDWR) {
        file->f_mode |= FMODE_WRITE;
    }

    if (file->f_op && file->f_op->open) {
        int ret = file->f_op->open(node, file);
        if (ret < 0) {
            file_put(file);
            inode_put(node);
            return ret;
        }
    }

    return fd;
}

SYSCALL_ATTR int sys_close(int fd)
{
    file_t* file = fd_get(fd);
    if (!file) {
        return -1;
    }

    myproc()->open_files[fd] = NULL;

    if (file->f_op && file->f_op->release) {
        file->f_op->release(file->f_inode, file);
    }

    file_put(file);
    return 0;
}

SYSCALL_ATTR int sys_unlink(char const* path)
{
    vfs_inode_t* root = myproc()->root;
    if (!root->i_op || !root->i_op->unlink) {
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

    // if (IS_RDONLY(dir)) {
    //     inode_put(parent);
    //     return -EROFS;
    // }

    if (!vfs_permission(parent, MAY_WRITE | MAY_EXEC)) {
        inode_put(parent);
        return -EACCES;
    }

    int error = root->i_op->unlink(parent, name, name_len);

    inode_put(parent);
    return error;
}

SYSCALL_ATTR int sys_chdir(char const* path)
{

    vfs_inode_t* base = NULL;
    if (path[0] == '/') {
        vfs_inode_t* root = myproc()->root;

        base = inode_get(root->i_sb, root->i_ino);
    } else {
        base = inode_get(myproc()->pwd->i_sb, myproc()->pwd->i_ino);
    }

    if (!base) {
        return -EIO;
    }

    vfs_inode_t* node = vfs_lookup(base, path);
    inode_put(base);
    if (!node) {
        return -ENOENT;
    }

    if (!S_ISDIR(node->i_mode)) {
        inode_put(node);
        return -ENOTDIR;
    }

    if (!vfs_permission(node, MAY_EXEC)) {
        inode_put(node);
        return -EACCES;
    }

    inode_put(myproc()->pwd);
    myproc()->pwd = node;

    return 0;
}

SYSCALL_ATTR int sys_mknod(char const* path, mode_t mode, dev_t dev)
{
    if (S_ISDIR(mode) || (!S_ISFIFO(mode) && myproc()->euid != ROOT_UID)) {
        return -EPERM;
    }

    switch (mode & S_IFMT) {
    case 0:
        mode |= S_IFREG;
        break;

    case S_IFREG:
    case S_IFCHR:
    case S_IFBLK:
        break;

    default:
        return -EINVAL;
    }

    return vfs_mknod(path, mode, dev);
}

SYSCALL_ATTR int sys_stat(char const* filename, struct stat* statbuf)
{
    vfs_inode_t* node = vfs_lookup(myproc()->root, filename);
    if (!node) {
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

SYSCALL_ATTR int sys_fstat(int fd, struct stat* statbuf)
{
    file_t* file = fd_get(fd);
    if (!file) {
        return -ENOENT;
    }

    struct stat tmp;
    vfs_inode_t* node = file->f_inode;

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
    file_t* file = fd_get((int)fd);
    if (!file) {
        return -EBADF;
    }

    if (file->f_op && file->f_op->lseek) {
        return file->f_op->lseek(file->f_inode, file, offset, (int)whence);
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

SYSCALL_ATTR int sys_readdir(u32 fd, dirent_t* dirent, int count)
{
    file_t* file = fd_get((int)fd);
    if (!file) {
        return -EBADF;
    }

    int result = -1;

    if (file->f_op && file->f_op->readdir) {
        result = file->f_op->readdir(file->f_inode, file, dirent, count);
    }

    return result;
}

SYSCALL_ATTR int sys_mkdir(char const* pathname, int mode)
{
    if (!pathname) {
        return -EINVAL;
    }

    size_t len = strlen(pathname);
    while (len > 1 && pathname[len - 1] == '/') {
        len--;
    }

    char clean_path[256];
    memcpy(clean_path, pathname, len);
    clean_path[len] = '\0';

    vfs_inode_t* node = vfs_lookup(myproc()->root, clean_path);
    if (node) {
        inode_put(node);

        return -EEXIST;
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

    vfs_inode_t* parent = vfs_lookup(myproc()->root, parent_path);
    if (!parent) {
        return -ENOENT;
    }

    // if (IS_RDONLY(parent)) {
    //     iput(dir);
    //     return -EROFS;
    // }

    if (!vfs_permission(parent, MAY_WRITE | MAY_EXEC)) {
        inode_put(parent);
        return -EACCES;
    }

    if (!parent->i_op || !parent->i_op->mkdir) {
        inode_put(parent);
        return -ENOTDIR;
    }

    int result = parent->i_op->mkdir(
        parent, name, (int)name_len, S_IFDIR | (mode & 0777)
    );

    inode_put(parent);
    return result;
}

SYSCALL_ATTR int sys_rmdir(char const* path)
{
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

    vfs_inode_t* parent = vfs_lookup(myproc()->root, parent_path);
    if (!parent) {
        return -ENOTDIR;
    }

    // if (IS_RDONLY(dir)) {
    //     iput(dir);
    //     return -EROFS;
    // }

    if (!vfs_permission(parent, MAY_WRITE | MAY_EXEC)) {
        inode_put(parent);
        return -EACCES;
    }

    if (!parent->i_op || !parent->i_op->rmdir) {
        inode_put(parent);
        return -ENOTDIR;
    }

    int retval = parent->i_op->rmdir(parent, name, (int)name_len);
    inode_put(parent);

    return retval;
}

SYSCALL_ATTR int sys_fchdir(int fd)
{
    file_t* file = fd_get(fd);
    if (!file) {
        return -EBADF;
    }

    if (!file->f_inode || !S_ISDIR(file->f_inode->i_mode)) {
        return -ENOTDIR;
    }

    if (!vfs_permission(file->f_inode, MAY_EXEC)) {
        return -EACCES;
    }

    inode_put(myproc()->pwd);
    myproc()->pwd = file->f_inode;

    file->f_inode->i_count += 1;
    return 0;
}

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

SYSCALL_ATTR int sys_getcwd(char* buf, unsigned long size)
{
    if (!buf || size == 0) {
        return -EINVAL;
    }

    vfs_inode_t* root = myproc()->root;
    vfs_inode_t* pwd = myproc()->pwd;

    if (!pwd) {
        pwd = root;
        pwd->i_count += 1;
    }

    if (pwd == root) {
        if (size < 2) {
            return -ERANGE;
        }
        buf[0] = '/';
        buf[1] = '\0';
        return 2;
    }

    u8 tmp[256];
    int pos = 255;
    tmp[pos] = '\0';

    vfs_inode_t* current = inode_get(pwd->i_sb, pwd->i_ino);
    if (!current) {
        return -EIO;
    }

    while (current != root) {
        vfs_inode_t* parent = vfs_lookup(current, "..");
        if (!parent) {
            inode_put(current);
            return -EIO;
        }

        if (parent->i_sb == current->i_sb && parent->i_ino == current->i_ino) {
            inode_put(parent);

            vfs_mount_t* mountpoint = find_mount(current);
            if (!mountpoint) {
                inode_put(current);
                return -EIO;
            }

            inode_put(current);
            current = mountpoint->m_root_inode;
            continue;
        }

        ext2_entry_t* entry = NULL;
        int retval = ext2_find_entry_by_ino(parent, current->i_ino, &entry);
        if (retval) {
            inode_put(current);
            inode_put(parent);
            return retval;
        }

        if (pos < entry->name_len + 1) {
            kfree(entry);
            inode_put(current);
            inode_put(parent);
            return -ENAMETOOLONG;
        }

        pos -= entry->name_len;
        memcpy(tmp + pos, entry->name, entry->name_len);
        kfree(entry);

        pos -= 1;
        tmp[pos] = '/';

        inode_put(current);
        current = parent;
    }

    inode_put(current);

    unsigned long path_len = 256 - pos;
    if (path_len > size) {
        return -ERANGE;
    }

    memcpy(buf, tmp + pos, path_len);
    return path_len;
}
