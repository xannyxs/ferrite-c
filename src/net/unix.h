#ifndef UNIX_H
#define UNIX_H

#include "types.h"

#define AF_UNIX 1
#define UNIX_PATH_MAX 108

struct sockaddr_un {
    u16 sun_family;               /* AF_UNIX */
    char sun_path[UNIX_PATH_MAX]; /* pathname */
};

/* Unix socket private data */
typedef struct unix_sock {
    char path[UNIX_PATH_MAX];
    struct socket* peer;

    char buffer[4096];
    size_t buf_len;
} unix_sock_t;

extern struct proto_ops unix_stream_ops;

#endif
