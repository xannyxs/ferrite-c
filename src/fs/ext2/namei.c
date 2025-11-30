#include "fs/ext2/ext2.h"
#include "fs/stat.h"
#include "fs/vfs.h"
#include "lib/math.h"
#include "memory/kmalloc.h"

#include <ferrite/errno.h>
#include <ferrite/types.h>
#include <lib/string.h>

int ext2_create(
    vfs_inode_t* dir,
    char const* name,
    int len,
    int mode,
    vfs_inode_t** result
)
{
    if (!dir) {
        return -1;
    }

    s32 err = 0;
    vfs_inode_t* new = ext2_new_inode(dir, mode, &err);
    if (!new) {
        return err;
    }

    ext2_entry_t entry = { 0 };
    entry.inode = new->i_ino;
    entry.name_len = len;
    entry.file_type = 0;
    memcpy(entry.name, name, entry.name_len);

    entry.rec_len = ALIGN(sizeof(ext2_entry_t) + entry.name_len, 4);
    if (ext2_write_entry(dir, &entry) < 0) {
        ext2_free_inode(new);
        inode_put(new);
        return -1;
    }

    if (dir->i_sb->s_op->write_inode(dir) < 0) {
        inode_put(new);
        return -1;
    }

    if (dir->i_sb->s_op->write_inode(new) < 0) {
        inode_put(new);
        return -1;
    }

    new->i_op = &ext2_file_inode_operations;

    *result = new;
    return 0;
}

/*
 * Consumes reference to inode (always calls inode_put).
 * On success, returns new reference in *result.
 */
s32 ext2_lookup(
    struct vfs_inode* inode,
    char const* name,
    int len,
    struct vfs_inode** result
)
{
    *result = NULL;

    if (!inode) {
        return -EINVAL;
    }

    if (!S_ISDIR(inode->i_mode)) {
        inode_put(inode);
        return -ENOTDIR;
    }

    ext2_entry_t* entry = NULL;
    if (ext2_find_entry(inode, name, len, &entry) < 0) {
        inode_put(inode);
        return -ENOENT;
    }
    unsigned long ino = entry->inode;
    kfree(entry);

    *result = inode_get(inode->i_sb, ino);
    if (!*result) {
        inode_put(inode);
        return -ENOMEM;
    }

    inode_put(inode);
    return 0;
}
