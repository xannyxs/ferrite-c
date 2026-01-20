#include "net/unix.h"
#include "drivers/printk.h"
#include "fs/stat.h"
#include "fs/vfs.h"
#include "memory/kmalloc.h"
#include "net/socket.h"
#include "sys/process/process.h"

#include <ferrite/errno.h>
#include <ferrite/string.h>
#include <types.h>

#define MAX_UNIX_SOCKETS 64

typedef struct unix_socket_node {
    socket_t* socket;
    struct unix_socket_node* next;
} unix_socket_node_t;

static unix_socket_node_t* unix_sockets_head = NULL;

static int unix_create(socket_t* s, int protocol);
static int unix_recvmsg(socket_t* s, void* buf, size_t len);
static int unix_sendmsg(socket_t* s, void const* buf, size_t len);
static int unix_bind(socket_t* s, void* addr, s32 addrlen);
static int unix_listen(socket_t* s, int backlog);
static int unix_connect(socket_t* s, void* addr, int len);
static int unix_accept(socket_t* s, socket_t* newsock);
static int unix_shutdown(socket_t* s, int how);

struct proto_ops unix_stream_ops = {
    .create = unix_create,
    .bind = unix_bind,
    .listen = unix_listen,
    .read = unix_recvmsg,
    .write = unix_sendmsg,
    .accept = unix_accept,
    .connect = unix_connect,
    .shutdown = unix_shutdown,
};

static int unix_register_socket(socket_t* sock)
{
    unix_socket_node_t* node = kmalloc(sizeof(unix_socket_node_t));
    if (!node) {
        return -ENOMEM;
    }

    node->socket = sock;
    node->next = unix_sockets_head;
    unix_sockets_head = node;

    return 0;
}

int unix_unregister_socket(socket_t* sock)
{
    unix_socket_node_t** current = &unix_sockets_head;

    while (current != NULL) {
        if ((*current)->socket == sock) {
            unix_socket_node_t* to_free = *current;
            *current = (*current)->next;
            kfree(to_free);
            return 0;
        }

        current = &(*current)->next;
    }

    return -1;
}

static int unix_create(socket_t* s, int protocol)
{
    (void)protocol;

    unix_sock_t* usock = kmalloc(sizeof(unix_sock_t));
    if (!usock) {
        return -1;
    }

    memset(usock, 0, sizeof(unix_sock_t));
    s->data = usock;

    return 0;
}

static int unix_listen(socket_t* s, int backlog)
{
    (void)backlog;

    unix_sock_t* usock = (unix_sock_t*)s->data;
    if (usock->path[0] == '\0') {
        return -1;
    }

    if (s->state != SS_UNCONNECTED) {
        return -1;
    }

    return 0;
}

static int unix_bind(socket_t* s, void* addr, s32 addrlen)
{
    struct sockaddr_un* sun = (struct sockaddr_un*)addr;
    unix_sock_t* usock = (unix_sock_t*)s->data;

    if ((u32)addrlen < sizeof(struct sockaddr_un) || sun->sun_family != AF_UNIX
        || usock->path[0] != '\0') {
        return -EINVAL;
    }

    char* path = sun->sun_path;
    char* last_slash = strrchr(path, '/');
    if (!last_slash) {
        return -EINVAL;
    }

    char dir_path[256];
    memcpy(dir_path, path, last_slash - path);
    dir_path[last_slash - path] = '\0';
    char* filename = last_slash + 1;
    size_t name_len = strlen(filename);

    vfs_inode_t* parent = vfs_lookup(myproc()->root, dir_path);
    if (!parent) {
        return -ENOENT;
    }

    if (!S_ISDIR(parent->i_mode)) {
        inode_put(parent);
        return -ENOTDIR;
    }

    vfs_inode_t* existing = vfs_lookup(parent, filename);
    if (existing) {
        inode_put(existing);
        inode_put(parent);
        return -EADDRINUSE;
    }

    if (!parent->i_op || !parent->i_op->create) {
        inode_put(parent);
        return -EPERM;
    }

    vfs_inode_t* sock_inode = NULL;
    int err = parent->i_op->create(
        parent, filename, (s32)name_len, S_IFSOCK | 0666, &sock_inode
    );

    inode_put(parent);

    if (err < 0) {
        return err;
    }

    sock_inode->u.i_socket = s;
    s->inode = sock_inode;

    strlcpy(usock->path, sun->sun_path, UNIX_PATH_MAX);

    if (unix_register_socket(s) < 0) {
        usock->path[0] = '\0';
        return -1;
    }

    printk("node: 0x%x\n", (u32)sock_inode);

    return 0;
}

static int unix_connect(socket_t* s, void* addr, int len)
{
    struct sockaddr_un* sun = addr;
    if ((u32)len < sizeof(struct sockaddr_un) || sun->sun_family != AF_UNIX) {
        return -EINVAL;
    }

    vfs_inode_t* node = vfs_lookup(myproc()->root, sun->sun_path);
    if (!node) {
        return -ENOENT;
    }

    printk(
        "[CONNECT] Found inode=%x, i_ino=%d, i_sb=%x\n", (u32)node,
        (int)node->i_ino, (u32)node->i_sb
    );
    printk("[CONNECT] node->u.i_socket=%x\n", (u32)node->u.i_socket);

    if (!S_ISSOCK(node->i_mode)) {
        inode_put(node);
        return -ECONNREFUSED;
    }

    socket_t* server = node->u.i_socket;
    if (server->conn) {
        return -1;
    }

    printk(
        "[CONNECT] server=%x, server->ops=%x, server->state=%d\n", (u32)server,
        (u32)server->ops, server->state
    );

    s->conn = server;
    server->conn = s;
    s->state = SS_CONNECTING;

    return 0;
}

static int unix_accept(socket_t* s, socket_t* newsock)
{
    while (!s->conn) {
        yield();
    }

    socket_t* client = s->conn;
    unix_sock_t* new_usock = kmalloc(sizeof(unix_sock_t));
    if (!new_usock) {
        return -ENOMEM;
    }

    memset(new_usock, 0, sizeof(unix_sock_t));
    newsock->data = new_usock;
    newsock->type = s->type;
    newsock->state = SS_CONNECTED;
    newsock->ops = s->ops;
    newsock->conn = client;

    client->state = SS_CONNECTED;
    client->conn = newsock;

    s->conn = NULL;

    return 0;
}

static int unix_recvmsg(socket_t* s, void* buf, size_t len)
{
    unix_sock_t* usock = (unix_sock_t*)s->data;
    if (s->state != SS_CONNECTED || usock->buf_len == 0) {
        return -1;
    }

    size_t to_read = len < usock->buf_len ? len : usock->buf_len;
    memcpy(buf, usock->buffer, to_read);

    if (to_read < usock->buf_len) {
        memmove(
            usock->buffer, usock->buffer + to_read, usock->buf_len - to_read
        );
    }

    usock->buf_len -= to_read;

    return (s32)to_read;
}

static int unix_sendmsg(socket_t* s, void const* buf, size_t len)
{
    printk(
        "  [SENDMSG] socket=0x%x, state=%d, conn=0x%x\n", (u32)s, s->state,
        (u32)s->conn
    );

    if (s->state != SS_CONNECTED) {
        return -1;
    }

    socket_t* peer = s->conn;
    if (!peer) {
        return -1;
    }

    unix_sock_t* peer_usock = (unix_sock_t*)peer->data;

    size_t to_copy = len;
    if (to_copy > sizeof(peer_usock->buffer) - peer_usock->buf_len) {
        to_copy = sizeof(peer_usock->buffer) - peer_usock->buf_len;
    }

    if (to_copy == 0) {
        return 0;
    }

    memcpy(peer_usock->buffer + peer_usock->buf_len, buf, to_copy);
    peer_usock->buf_len += to_copy;

    return (s32)to_copy;
}

static int unix_shutdown(socket_t* s, int how)
{
    (void)how;

    s->state = SS_DISCONNECTING;
    s->conn = NULL;

    return 0;
}
