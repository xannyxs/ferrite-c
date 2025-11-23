#include "drivers/printk.h"
#include "ferrite/dirent.h"
#include "ferrite/types.h"
#include "fs/ext2/ext2.h"
#include "fs/vfs.h"
#include "sys/file/file.h"
#include "sys/file/stat.h"

#include <lib/string.h>

static s32
ext2_dir_read(vfs_inode_t* inode, struct file* file, void* buff, int count)
{
    (void)inode;
    (void)file;
    (void)buff;
    (void)count;

    // return -EISDIR;
    return -1;
}

static s32
ext2_readdir(vfs_inode_t* inode, file_t* file, dirent_t* dirent, s32 count);

static int
ext2_mkdir(struct vfs_inode* dir, char const* name, int len, int mode);

static struct file_operations ext2_dir_operations = {
    .write = NULL,
    .readdir = ext2_readdir,
    .read = ext2_dir_read,
};

struct inode_operations ext2_dir_inode_ops = {
    .default_file_ops = &ext2_dir_operations,
    // .create = ext2_create,
    .lookup = ext2_lookup,
    .mkdir = ext2_mkdir,
    //     ext2_link,            /* link */
    //     ext2_unlink,          /* unlink */
    //     ext2_symlink,         /* symlink */
    //     ext2_rmdir,           /* rmdir */
    //     ext2_mknod,           /* mknod */
    //     ext2_rename,          /* rename */
    //     NULL,                 /* readlink */
    //     NULL,                 /* follow_link */
    //     NULL,                 /* bmap */
    //     ext2_truncate,        /* truncate */
    //     ext2_permission       /* permission */
};

s32 ext2_readdir(vfs_inode_t* inode, file_t* file, dirent_t* dirent, s32 count)
{
    (void)count;
    ext2_entry_t* entry;

    if (!inode || !S_ISDIR(inode->i_mode)) {
        return -1;
    }

    vfs_superblock_t* sb = inode->i_sb;
    ext2_super_t* es = sb->u.ext2_sb.s_es;

    while (file->f_pos < (long)inode->i_size) {
        unsigned long offset = file->f_pos & (sb->s_blocksize - 1);
        unsigned long block = (file->f_pos) >> (es->s_log_block_size + 10);

        if (block >= 12) {
            return -1;
        }

        u32 block_num = inode->u.i_ext2->i_block[block];
        u8 buff[sb->s_blocksize];

        if (ext2_read_block(inode, buff, block_num) < 0) {
            return -1;
        }

        entry = (ext2_entry_t*)&buff[offset];
        while (offset < sb->s_blocksize && file->f_pos < (long)inode->i_size) {
            if (entry->rec_len == 0 || entry->rec_len > sb->s_blocksize) {
                return -1;
            }

            offset += entry->rec_len;
            file->f_pos += entry->rec_len;

            if (entry->inode) {
                memcpy(dirent->d_name, entry->name, entry->name_len);
                dirent->d_name[entry->name_len] = '\0';

                dirent->d_ino = (long)entry->inode;
                dirent->d_reclen = entry->name_len;
                dirent->d_off = file->f_pos;

                return entry->name_len;
            }

            entry = (ext2_entry_t*)((char*)entry + entry->rec_len);
        }
    }

    return 0;
}

int ext2_mkdir(struct vfs_inode* dir, char const* name, int len, int mode)
{
    (void)name;
    (void)len;
    if (!dir) {
        return -1;
    }

    if (!S_ISDIR(mode)) {
        return -1;
    }

    ext2_entry_t* entry = NULL;
    if (ext2_find_entry(dir, name, len, &entry) < 0) {
        inode_put(dir);
        return -1;
    }

    // s32 node_num = find_free_inode(m);
    // if (node_num < 0) {
    //     return -1;
    // }
    //
    // if (mark_inode_allocated(m, node_num) < 0) {
    //     return -1;
    // }
    //
    // s32 block_num = find_free_block(m);
    // if (block_num < 0) {
    //     return -1;
    // }
    //
    // if (mark_block_allocated(m, block_num) < 0) {
    //     return -1;
    // }

    // node->i_mode = mode;
    // node->i_uid = 0;
    //
    // node->i_size = m->m_block_size;
    // node->i_blocks = (1 * m->m_block_size) / 512;
    // node->i_block[0] = block_num;
    // for (int i = 1; i < 15; i += 1) {
    //     node->i_block[i] = 0;
    // }
    //
    // u32 now = getepoch();
    // node->i_atime = now;
    // node->i_ctime = now;
    // node->i_mtime = now;
    // node->i_dtime = 0;
    //
    // node->i_gid = 0;
    // node->i_links_count = 2;
    // node->i_flags = 0;
    //
    // new->i_ino = node_num;
    // new->i_mode = mode;
    // new->i_size = m->m_block_size;
    // // new->i_op = &ext2_dir_operations;
    // new->u.i_ext2 = node;
    //
    // ext2_entry_t entry = { 0 };
    // entry.inode = node_num;
    // entry.name_len = strlen(dentry->d_name);
    // entry.file_type = 0;
    // memcpy(entry.name, dentry->d_name, entry.name_len);
    //
    // entry.rec_len = sizeof(ext2_entry_t) + entry.name_len;
    // if (ext2_entry_write(m, &entry, parent->i_ino) < 0) {
    //     kfree(node);
    //     kfree(new);
    //     return -1;
    // }
    //
    // if (ext2_write_inode(m, node_num, node) < 0) {
    //     kfree(node);
    //     kfree(new);
    //     return -1;
    // }
    //
    // u8 buff[m->m_block_size];
    //
    // char const* current_str = ".";
    // ext2_entry_t current_entry = { 0 };
    // current_entry.inode = node_num;
    // current_entry.name_len = strlen(current_str);
    // current_entry.file_type = 0;
    // memcpy(current_entry.name, current_str, current_entry.name_len);
    //
    // current_entry.rec_len
    //     = ALIGN(sizeof(ext2_entry_t) + current_entry.name_len, 4);
    // memcpy(buff, &current_entry, current_entry.rec_len);
    //
    // char const* parent_str = "..";
    // ext2_entry_t parent_entry = { 0 };
    // parent_entry.inode = parent->i_ino;
    // parent_entry.name_len = strlen(parent_str);
    // parent_entry.file_type = 0;
    // memcpy(parent_entry.name, parent_str, parent_entry.name_len);
    //
    // parent_entry.rec_len = m->m_block_size - current_entry.rec_len;
    // memcpy(buff + current_entry.rec_len, &parent_entry,
    // parent_entry.rec_len);
    //
    // block_device_t* d = m->m_device;
    // u32 const addr = node->i_block[0] * m->m_block_size;
    // u32 sector_pos = addr / d->sector_size;
    // u32 const count = m->m_block_size / d->sector_size;
    //
    // if (d->d_op->write(d, sector_pos, count, buff, m->m_block_size) < 0)
    // {
    //     return -1;
    // }
    //
    // ext2_inode_t parent_inode = { 0 };
    // if (ext2_read_inode(m, parent->i_ino, &parent_inode) < 0) {
    //     kfree(node);
    //     kfree(new);
    //     return -1;
    // }
    //
    // parent_inode.i_links_count += 1;
    // parent_inode.i_ctime = now;
    // parent_inode.i_mtime = now;
    //
    // if (ext2_write_inode(m, parent->i_ino, &parent_inode) < 0) {
    //     kfree(node);
    //     kfree(new);
    //     return -1;
    // }
    //
    // dentry->d_inode = new;
    return 0;
}
