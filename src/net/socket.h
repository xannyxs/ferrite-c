#ifndef SOCKET_H
#define SOCKET_H

#include "sys/file/file.h"
#include <ferrite/types.h>

#define SOCK_STREAM 1 /* Stream socket (TCP, Unix STREAM) */
#define SOCK_DGRAM 2  /* Datagram socket (UDP, Unix DGRAM) */

struct vfs_inode;

typedef enum {
    SS_FREE = 0,     /* not allocated		*/
    SS_UNCONNECTED,  /* unconnected to any socket	*/
    SS_CONNECTING,   /* in process of connecting	*/
    SS_CONNECTED,    /* connected to socket		*/
    SS_DISCONNECTING /* in process of disconnecting	*/
} socket_state_e;

typedef struct socket {
    s16 type;
    socket_state_e state; /* Connection state */

    struct proto_ops* ops;   /* Protocol operations (TCP/UDP) */
    void* data;              /* Protocol-specific data */
    struct vfs_inode* inode; /* Back pointer to inode */

    struct socket* conn;
} socket_t;

struct proto_ops {
    int (*create)(socket_t* s, s32 protocol);
    int (*read)(socket_t* s, void* buf, size_t len);
    int (*write)(socket_t* s, void const* buf, size_t len);
    int (*bind)(socket_t* s, void* addr, s32 addrlen);
    int (*listen)(socket_t* s, int backlog);
    int (*connect)(socket_t* s, void* addr, int len);
    int (*accept)(socket_t* s, socket_t* newsock);
    int (*shutdown)(socket_t* sock, int how);
};

struct vfs_inode* __alloc_socket(socket_t* _s);

s32 socket_create(s32 family, s16 type, s32 protocol);

s32 socket_close(file_t* f);

#endif
