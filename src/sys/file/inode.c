#include "sys/file/inode.h"
#include "memory/kmalloc.h"
#include "net/socket.h"
#include "sys/file/stat.h"
#include "types.h"

inode_t* inode_get(u16 mode)
{
    inode_t* inode = kmalloc(sizeof(inode_t));
    if (!inode) {
        return NULL;
    }

    inode->i_count = 0;
    inode->i_mode = mode;

    return inode;
}

void inode_put(inode_t* n)
{
    if (!n) {
        return;
    }

    n->i_count -= 1;
    if (n->i_count == 0) {
        if (S_ISSOCK(n->i_mode) && n->u.i_socket) {
            socket_t* sock = n->u.i_socket;

            if (sock->data) {
                kfree(sock->data);
            }

            kfree(sock);
        }
        kfree(n);
    }
}
