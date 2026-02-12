#ifndef _KERNEL_UNIX_H
#define _KERNEL_UNIX_H

#include <uapi/socket.h>
#include <uapi/types.h>

#define UNIX_PATH_MAX 108

extern struct proto_ops unix_stream_ops;

typedef struct unix_sock {
    char path[UNIX_PATH_MAX];
    struct socket* peer;

    char buffer[4096];
    size_t buf_len;
} unix_sock_t;

int unix_unregister_socket(socket_t*);

#endif
