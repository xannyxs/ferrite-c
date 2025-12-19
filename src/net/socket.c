#include "net/socket.h"
#include "arch/x86/time/time.h"
#include "fs/stat.h"
#include "fs/vfs.h"
#include "memory/kmalloc.h"
#include "net/unix.h"
#include "sys/file/fcntl.h"
#include "sys/file/file.h"
#include "sys/process/process.h"

#include <ferrite/errno.h>
#include <ferrite/types.h>
#include <lib/stdlib.h>
#include <ferrite/string.h>

static int socket_read(vfs_inode_t* node, file_t* file, void* buf, int len);
static int
socket_write(vfs_inode_t* node, file_t* file, void const* buf, int len);
static void socket_release(vfs_inode_t* node, file_t* file);

static struct file_operations socket_file_operations
    = { .read = socket_read,
        .write = socket_write,
        .release = socket_release,

        .open = NULL,
        .readdir = NULL,
        .lseek = NULL };

static struct inode_operations socket_inode_operations = {
    .default_file_ops = &socket_file_operations,

    .create = NULL, // Should I create a function for this?
    .lookup = NULL,
    .inode_get = NULL,
    .inode_put = NULL,
    .mkdir = NULL,
    .rmdir = NULL,
    .unlink = NULL,
    .truncate = NULL,
};

static int socket_read(vfs_inode_t* node, file_t* file, void* buf, int len)
{
    (void)file;

    if (!node || !S_ISSOCK(node->i_mode)) {
        return -1;
    }

    socket_t* s = node->u.i_socket;
    if (!s) {
        return -1;
    }

    if (s->ops && s->ops->read) {
        return s->ops->read(s, buf, len);
    }

    return -1;
}

static int
socket_write(vfs_inode_t* node, file_t* file, void const* buf, int len)
{
    (void)file;
    if (!node || !S_ISSOCK(node->i_mode)) {
        return -1;
    }

    socket_t* s = node->u.i_socket;
    if (!s) {
        return -1;
    }

    if (s->ops && s->ops->write) {
        return s->ops->write(s, buf, len);
    }

    return -1;
}

void socket_release(vfs_inode_t* node, file_t* file)
{
    (void)file;

    if (!node || !S_ISSOCK(node->i_mode)) {
        return;
    }

    socket_t* s = node->u.i_socket;
    if (!s) {
        return;
    }

    if (s->ops && s->ops->shutdown) {
        s->ops->shutdown(s, 0);
    }

    if (s->data) {
        unix_sock_t* usock = (unix_sock_t*)s->data;
        if (usock->path[0] != '\0') {
            unix_unregister_socket(s);
            usock->path[0] = '\0';
        }
    }
}

s32 socket_create(s32 family, s16 type, s32 protocol)
{
    socket_t* s = kmalloc(sizeof(socket_t));
    if (!s) {
        return -1;
    }

    memset(s, 0, sizeof(socket_t));
    s->type = type;
    s->state = SS_UNCONNECTED;
    s->data = NULL;
    s->conn = NULL;

    if (family == AF_UNIX) {
        if (type == SOCK_STREAM) {
            s->ops = &unix_stream_ops;
        } else {
            kfree(s);
            return -ENOSYS;
        }
    } else {
        kfree(s);
        return -ENOSYS;
    }

    if (!s->ops || !s->ops->create) {
        kfree(s);
        return -1;
    }

    if (s->ops->create(s, protocol) < 0) {
        kfree(s);
        return -1;
    }

    vfs_inode_t* node = __alloc_socket(s);
    if (!node) {
        if (s->data) {
            kfree(s->data);
        }
        kfree(s);
        return -1;
    }

    int fd = fd_alloc();
    if (fd < 0) {
        // TODO: Leaking? I need to check if I can free the union safely

        inode_put(node);
        return fd;
    }

    file_t* file = fd_get(fd);
    if (!file) {
        abort("Something went seriously wrong!");
    }

    file->f_inode = node;
    file->f_op = &socket_file_operations;

    file->f_pos = 0;
    file->f_mode = FMODE_READ | FMODE_WRITE;
    file->f_flags = O_RDWR;
    file->f_count = 1;

    return fd;
}

/* Public */

vfs_inode_t* __alloc_socket(socket_t* _s)
{
    if (!_s) {
        return NULL;
    }

    vfs_inode_t* node = inode_get_empty(NULL, 0);
    if (!node) {
        return NULL;
    }

    time_t now = getepoch();
    node->i_ctime = now;
    node->i_mtime = now;
    node->i_atime = now;

    node->i_mode = S_IFSOCK | 0666;
    node->i_dev = 0;
    node->i_size = 0;
    node->i_links_count = 1;

    node->i_uid = myproc()->uid;
    node->i_gid = myproc()->gid;

    node->i_op = &socket_inode_operations;

    node->u.i_socket = _s;
    _s->inode = node;

    return node;
}
