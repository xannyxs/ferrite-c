#include "fs/vfs.h"
#include "fs/vfs/mode_t.h"
#include "memory/kmalloc.h"
#include "net/socket.h"
#include "sys/file/stat.h"

vfs_inode_t* inode_get(mode_t mode)
{
    vfs_inode_t* inode = kmalloc(sizeof(vfs_inode_t));
    if (!inode) {
        return NULL;
    }

    inode->i_count = 0;
    inode->i_mode = mode;

    return inode;
}

void inode_put(vfs_inode_t* n)
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
