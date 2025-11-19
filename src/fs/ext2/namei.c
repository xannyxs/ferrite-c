#include "fs/ext2/ext2.h"
#include "fs/vfs.h"
#include "memory/kmalloc.h"
#include "sys/file/stat.h"

#include <ferrite/types.h>
#include <lib/string.h>

// int ext2_create(struct vfs_inode* parent, struct vfs_dentry* dentry, int
// mode)
// {
//     if (!parent) {
//         return -1;
//     }
//
//     ext2_mount_t* m = &ext2_mounts[0];
//
//     s32 node_num = find_free_inode(m);
//     if (node_num < 0) {
//         return -1;
//     }
//
//     if (mark_inode_allocated(m, node_num) < 0) {
//         return -1;
//     }
//
//     struct vfs_inode* new = kmalloc(sizeof(struct vfs_inode));
//     if (!new) {
//         return -1;
//     }
//
//     ext2_inode_t* node = kmalloc(sizeof(ext2_inode_t));
//     if (!node) {
//         kfree(new);
//         return -1;
//     }
//
//     node->i_mode = mode;
//     node->i_uid = 0;
//     node->i_size = 0;
//
//     u32 now = getepoch();
//     node->i_atime = now;
//     node->i_ctime = now;
//     node->i_mtime = now;
//     node->i_dtime = 0;
//
//     node->i_gid = 0;
//     node->i_links_count = 1;
//     node->i_flags = 0;
//     node->i_blocks = 0;
//     for (int i = 0; i < 15; i += 1) {
//         node->i_block[i] = 0;
//     }
//
//     new->i_ino = node_num;
//     new->i_mode = mode;
//     new->i_size = 0;
//     // new->i_op = &ext2_fops;
//     new->u.i_ext2 = node;
//
//     ext2_entry_t entry = { 0 };
//     entry.inode = node_num;
//     entry.name_len = strlen(dentry->d_name);
//     entry.file_type = 0;
//     memcpy(entry.name, dentry->d_name, entry.name_len);
//
//     entry.rec_len = sizeof(ext2_entry_t) + entry.name_len;
//     if (ext2_entry_write(m, &entry, parent->i_ino) < 0) {
//         kfree(node);
//         kfree(new);
//         return -1;
//     }
//
//     // if (ext2_write_inode(node) < 0) {
//     //     kfree(node);
//     //     kfree(new);
//     //     return -1;
//     // }
//
//     dentry->d_inode = new;
//     return 0;
// }

s32 ext2_lookup(
    struct vfs_inode* inode,
    char const* name,
    int len,
    struct vfs_inode** result
)
{
    *result = NULL;

    if (!inode) {
        return -1;
    }

    if (!S_ISDIR(inode->i_mode)) {
        inode_put(inode);
        return -1;
    }

    ext2_entry_t* entry = NULL;
    if (ext2_find_entry(inode, name, len, &entry) < 0) {
        inode_put(inode);
        return -1;
    }
    unsigned long ino = entry->inode;
    kfree(entry);

    *result = inode_get(inode->i_sb, ino);
    if (!*result) {
        inode_put(inode);
        return -1;
    }

    inode_put(inode);
    return 0;
}
