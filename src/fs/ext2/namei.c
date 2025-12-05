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
        return -EINVAL;
    }

    s32 err = 0;
    vfs_inode_t* new = ext2_new_inode(dir, mode, &err);
    if (!new) {
        return err;
    }

    new->i_op = &ext2_file_inode_operations;

    err = dir->i_sb->s_op->write_inode(new);
    if (err < 0) {
        ext2_free_inode(new);
        inode_put(new);
        return err;
    }

    ext2_entry_t entry = { 0 };
    entry.inode = new->i_ino;
    entry.name_len = len;
    entry.file_type = 0;
    memcpy(entry.name, name, entry.name_len);
    entry.rec_len = ALIGN(sizeof(ext2_entry_t) + entry.name_len, 4);

    err = ext2_write_entry(dir, &entry);
    if (err < 0) {
        ext2_free_inode(new);
        inode_put(new);
        return -err;
    }

    err = dir->i_sb->s_op->write_inode(dir);
    if (err < 0) {
        // NOTE: Can't rollback - directory entry already on disk

        inode_put(new);
        return err;
    }

    *result = new;

    return 0;
}

s32 ext2_lookup(
    struct vfs_inode* node,
    char const* name,
    int len,
    struct vfs_inode** result
)
{
    *result = NULL;

    if (!node) {
        return -EINVAL;
    }

    if (!S_ISDIR(node->i_mode)) {
        return -ENOTDIR;
    }

    ext2_entry_t* entry = NULL;
    if (ext2_find_entry(node, name, len, &entry) < 0) {
        return -ENOENT;
    }
    unsigned long ino = entry->inode;
    kfree(entry);

    *result = inode_get(node->i_sb, ino);
    if (!*result) {
        return -ENOMEM;
    }

    return 0;
}
