#include "fs/ext2/ext2.h"
#include "fs/stat.h"
#include "fs/vfs.h"
#include "memory/kmalloc.h"
#include "sys/process/process.h"

#include <lib/string.h>

int in_group_p(gid_t grp)
{
    int i;

    if (grp == myproc()->egid) {
        return 1;
    }

    for (i = 0; i < NGROUPS; i++) {
        if (myproc()->groups[i] == NOGROUP) {
            break;
        }

        if (myproc()->groups[i] == grp) {
            return 1;
        }
    }

    return 0;
}

int vfs_permission(vfs_inode_t* node, int mask)
{
    int mode = node->i_mode;

    if (node->i_op && node->i_op->permission) {
        return node->i_op->permission(node, mask);
    }

    if (myproc()->euid == node->i_uid) {
        mode >>= 6;
    } else if (in_group_p(node->i_gid)) {
        mode >>= 3;
    }

    if (((mode & mask & S_IRWXO) == mask) || myproc()->euid == ROOT_UID) {
        return 1;
    }

    return 0;
}

/*
 * @brief Lookup a path starting from the given inode.
 *
 * @note Does NOT release the start node - caller must call inode_put(start).
 */
vfs_inode_t* vfs_lookup(vfs_inode_t* start, char const* path)
{
    if (!start || !path) {
        return NULL;
    }

    if (path[0] == '/' && path[1] == '\0') {
        return inode_get(myproc()->root->i_sb, EXT2_ROOT_INO);
    }

    char** components = split(path, '/');
    if (!components) {
        return NULL;
    }

    vfs_inode_t* current = start;
    vfs_inode_t* new = NULL;
    for (int i = 0; components[i]; i += 1) {
        if (components[i][0] == '\0') {
            continue;
        }

        s32 name_len = (s32)strlen(components[i]);
        if (current->i_op->lookup(current, components[i], name_len, &new) < 0) {
            goto error;
        }

        current = new;
    }

    for (int i = 0; components[i]; i += 1) {
        kfree(components[i]);
    }
    kfree(components);

    return current;

error:
    for (int i = 0; components[i]; i += 1) {
        kfree(components[i]);
    }
    kfree(components);

    return NULL;
}
