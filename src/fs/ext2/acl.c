/*
 * ACL stands for Access Control Lists
 */

#include "fs/stat.h"
#include "fs/vfs.h"
#include "sys/process/process.h"

int ext2_permission(vfs_inode_t* node, int mask)
{
    u16 mode = node->i_mode;

    if (myproc()->euid == ROOT_UID) {
        return 1;
    }

    if (myproc()->euid == node->i_uid) {
        mode >>= 6;
    } else if (in_group_p(node->i_gid)) {
        mode >>= 3;
    }

    if ((mode & mask & S_IRWXO) == mask) {
        return 1;
    }

    return 0;
}
