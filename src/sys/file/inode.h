#ifndef INODE_H
#define INODE_H

#include "types.h"

struct socket;

typedef struct inode {
    u16 i_mode;
    u16 i_count;

    union {
        struct socket* i_socket;
        // struct file inode*; (WIP)
    } u;
} inode_t;

inode_t* inode_get(u16 mode);

void inode_put(inode_t* n);

#endif
