#include "net/unix.h"
#include "lib/string.h"
#include "memory/kmalloc.h"
#include "net/socket.h"
#include "types.h"

#define MAX_UNIX_SOCKETS 64

static socket_t* unix_sockets[MAX_UNIX_SOCKETS];

static int unix_create(socket_t* s, int protocol);
static int unix_recvmsg(socket_t* s, void* buf, size_t len);
static int unix_sendmsg(socket_t* s, void const* buf, size_t len);
static int unix_bind(socket_t* s, void* addr, s32 addrlen);
static int unix_listen(socket_t* s, int backlog);
static int unix_connect(socket_t* s, void* addr, int len);
static int unix_accept(socket_t* s, socket_t* newsock);

struct proto_ops unix_stream_ops = {
    .create = unix_create,
    .bind = unix_bind,
    .listen = unix_listen,
    .read = unix_recvmsg,
    .write = unix_sendmsg,
    .accept = unix_accept,
    .connect = unix_connect,
};

static socket_t* unix_find_socket(char const* sun_path)
{
    for (int i = 0; i < MAX_UNIX_SOCKETS; i++) {
        if (unix_sockets[i] != NULL) {
            unix_sock_t* usock = (unix_sock_t*)unix_sockets[i]->data;

            if (strcmp(usock->path, sun_path) == 0) {
                return unix_sockets[i];
            }
        }
    }

    return NULL;
}

static int unix_create(socket_t* s, int protocol)
{
    (void)unix_sockets;
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

    if (s->state != SS_CONNECTED) {
        return -1;
    }

    return 0;
}

static int unix_bind(socket_t* s, void* addr, s32 addrlen)
{
    struct sockaddr_un* sun = (struct sockaddr_un*)addr;
    unix_sock_t* usock = (unix_sock_t*)s->data;

    if ((u32)addrlen < sizeof(struct sockaddr_un)) {
        return -1;
    }
    if (sun->sun_family != AF_UNIX) {
        return -1;
    }

    strlcpy(usock->path, sun->sun_path, UNIX_PATH_MAX);

    return 0;
}

static int unix_connect(socket_t* s, void* addr, int len)
{
    struct sockaddr_un* sun = addr;
    if ((u32)addr < sizeof(struct sockaddr_un)) {
        return -1;
    }
    if (sun->sun_family != AF_UNIX) {
        return -1;
    }

    socket_t* server = unix_find_socket(sun->sun_path);
    if (!server) {
        return -1;
    }

    if (server->conn) {
        return -1;
    }

    s->conn = server;
    server->conn = s;
    server->state = SS_CONNECTED;

    return 0;
}

static int unix_accept(socket_t* s, socket_t* newsock)
{
    if (!s->conn) {
        return -1;
    }

    socket_t* client = s->conn;
    unix_sock_t* new_usock = kmalloc(sizeof(unix_sock_t));
    if (!new_usock) {
        return -1;
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
        memmove(usock->buffer, usock->buffer + to_read,
            usock->buf_len - to_read);
    }

    usock->buf_len -= to_read;

    return (s32)to_read;
}

static int unix_sendmsg(socket_t* s, void const* buf, size_t len)
{
    unix_sock_t* usock = (unix_sock_t*)s->data;
    if (s->state != SS_CONNECTED || !usock->peer) {
        return -1;
    }

    unix_sock_t* peer = (unix_sock_t*)usock->peer->data;
    size_t to_copy = len;
    if (to_copy > sizeof(peer->buffer) - peer->buf_len) {
        to_copy = sizeof(peer->buffer) - peer->buf_len;
    }

    if (to_copy == 0) {
        return 0;
    }

    memcpy(peer->buffer + peer->buf_len, buf, to_copy);
    peer->buf_len += to_copy;

    return (s32)to_copy;
}
