#include "net/socket.h"
#include "lib/string.h"
#include "memory/kmalloc.h"
#include "net/unix.h"
#include "sys/file/fcntl.h"
#include "sys/file/file.h"
#include "sys/file/inode.h"
#include "sys/file/stat.h"
#include "sys/process/process.h"
#include "types.h"

static int socket_read(struct file* f, void* buf, size_t len);
static int socket_write(struct file* f, void const* buf, size_t len);

static struct file_operations socket_fops = {
    .read = socket_read,
    .write = socket_write,
    .close = socket_close,
    .lseek = NULL
};

static int socket_read(struct file* f, void* buf, size_t len)
{
    if (!f->f_inode || !S_ISSOCK(f->f_inode->i_mode)) {
        return -1;
    }

    socket_t* s = f->f_inode->u.i_socket;
    if (!s) {
        return -1;
    }

    if (s->ops && s->ops->read) {
        return s->ops->read(s, buf, len);
    }

    return -1;
}

static int socket_write(struct file* f, void const* buf, size_t len)
{
    if (!f->f_inode || !S_ISSOCK(f->f_inode->i_mode)) {
        return -1;
    }

    socket_t* s = f->f_inode->u.i_socket;
    if (!s) {
        return -1;
    }

    if (s->ops && s->ops->write) {
        return s->ops->write(s, buf, len);
    }

    return -1;
}

s32 socket_close(struct file* f)
{
    if (!f || !S_ISSOCK(f->f_inode->i_mode)) {
        return -1;
    }

    socket_t* s = f->f_inode->u.i_socket;
    if (!s) {
        return -1;
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

    return 0;
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
            return -1;
        }
    } else {
        kfree(s);
        return -1;
    }

    if (!s->ops || !s->ops->create) {
        kfree(s);
        return -1;
    }

    if (s->ops->create(s, protocol) < 0) {
        kfree(s);
        return -1;
    }

    struct inode* inode = inode_get(S_IFSOCK | 0666);
    if (!inode) {
        if (s->data) {
            kfree(s->data);
        }
        kfree(s);
        return -1;
    }

    inode->i_count = 1;
    inode->u.i_socket = s;
    s->inode = inode;

    struct file* file = file_get();
    if (!file) {
        inode_put(inode);
        return -1;
    }

    file->f_inode = inode;
    file->f_count = 1;
    file->f_pos = 0;
    file->f_flags = O_RDWR;
    file->f_mode = FMODE_READ | FMODE_WRITE;
    file->f_op = &socket_fops;

    proc_t* p = myproc();
    if (!p) {
        file_put(file);
        return -1;
    }

    for (s32 fd = 0; fd < MAX_OPEN_FILES; fd += 1) {
        if (!p->open_files[fd]) {
            p->open_files[fd] = file;
            return fd;
        }
    }

    file_put(file);
    return -1;
}
