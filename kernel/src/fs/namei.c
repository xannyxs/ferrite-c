#include "ferrite/types.h"
#include "fs/ext2/ext2.h"
#include "fs/stat.h"
#include "fs/vfs.h"
#include "memory/kmalloc.h"
#include "sys/process/process.h"

#include <ferrite/errno.h>
#include <ferrite/string.h>

int dir_namei(
    char const* pathname,
    int* namelen,
    char const** basename,
    vfs_inode_t* base,
    vfs_inode_t** res_inode
)
{
    char* tmp;
    char const* thisname;
    int len;
    vfs_inode_t* inode;

    if (!base) {
        base = myproc()->root;
        base->i_count++;
    }

    tmp = strrchr(pathname, '/');
    if (!tmp) {
        thisname = pathname;
        len = strlen(thisname);
        inode = base;
    } else if (tmp[1] == '\0') {
        inode_put(base);
        return -EISDIR;
    } else {
        len = strlen(tmp + 1);
        thisname = tmp + 1;
        *tmp = '\0';

        inode = vfs_lookup(base, pathname);

        *tmp = '/';

        inode_put(base);

        if (!inode) {
            return -ENOENT;
        }

        if (!S_ISDIR(inode->i_mode)) {
            inode_put(inode);
            return -ENOTDIR;
        }
    }

    *namelen = len;
    *basename = thisname;
    *res_inode = inode;

    return 0;
}

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
    if (!vfs_permission(start, MAY_EXEC)) {
        return NULL;
    }
    if (path[0] == '/' && path[1] == '\0') {
        return inode_get(myproc()->root->i_sb, EXT2_ROOT_INO);
    }

    char** components = split(path, '/');
    if (!components) {
        return NULL;
    }

    vfs_inode_t* current = NULL;
    vfs_inode_t* new = NULL;

    for (int i = 0; components[i]; i += 1) {
        if (components[i][0] == '\0') {
            continue;
        }

        s32 name_len = (s32)strlen(components[i]);
        vfs_inode_t* parent = (current == NULL) ? start : current;

        if (parent->i_op->lookup(parent, components[i], name_len, &new) < 0) {
            inode_put(current);
            goto error;
        }

        inode_put(current);
        current = new;

        if (current->i_mount) {
            vfs_inode_t* mounted_root
                = inode_get(current->i_mount->i_sb, current->i_mount->i_ino);

            inode_put(current);
            current = mounted_root;
        }

        if (S_ISDIR(current->i_mode) && components[i + 1]) {
            if (!vfs_permission(current, MAY_EXEC)) {
                inode_put(current);
                goto error;
            }
        }
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

int vfs_mknod(char const* path, int mode, dev_t dev)
{
    char const* basename;
    int namelen, error;
    vfs_inode_t* dir;

    error = dir_namei(path, &namelen, &basename, NULL, &dir);
    if (error) {
        return error;
    }

    if (!namelen) {
        inode_put(dir);
        return -ENOENT;
    }

    if (!vfs_permission(dir, MAY_WRITE | MAY_EXEC)) {
        inode_put(dir);
        return -EACCES;
    }
    if (!dir->i_op || !dir->i_op->mknod) {
        inode_put(dir);
        return -EPERM;
    }

    error = dir->i_op->mknod(dir, basename, namelen, mode, dev);

    return error;
}
