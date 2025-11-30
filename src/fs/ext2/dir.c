#include "arch/x86/time/time.h"
#include "drivers/block/device.h"
#include "drivers/printk.h"
#include "fs/ext2/ext2.h"
#include "fs/stat.h"
#include "fs/vfs.h"
#include "lib/math.h"
#include "memory/kmalloc.h"
#include "sys/file/file.h"

#include <ferrite/dirent.h>
#include <ferrite/errno.h>
#include <ferrite/types.h>
#include <lib/string.h>

static s32
ext2_dir_read(vfs_inode_t* inode, struct file* file, void* buff, int count)
{
    (void)inode;
    (void)file;
    (void)buff;
    (void)count;

    return -EISDIR;
}

static s32
ext2_readdir(vfs_inode_t* inode, file_t* file, dirent_t* dirent, s32 count);

static int
ext2_mkdir(struct vfs_inode* dir, char const* name, int len, int mode);

static int ext2_rmdir(vfs_inode_t* dir, char const* name, int len);

static struct file_operations ext2_dir_operations = {
    .write = NULL,
    .readdir = ext2_readdir,
    .read = ext2_dir_read,
    .release = NULL,
    .open = NULL,
    .lseek = NULL,
};

struct inode_operations ext2_dir_inode_operations = {
    .default_file_ops = &ext2_dir_operations,
    .create = ext2_create,
    .lookup = ext2_lookup,
    .mkdir = ext2_mkdir,
    .rmdir = ext2_rmdir,
    //     ext2_link,            /* link */
    //     ext2_unlink,          /* unlink */
    //     ext2_symlink,         /* symlink */
    //     ext2_mknod,           /* mknod */
    //     ext2_rename,          /* rename */
    //     NULL,                 /* readlink */
    //     NULL,                 /* follow_link */
    //     NULL,                 /* bmap */
    //     ext2_truncate,        /* truncate */
    //     ext2_permission       /* permission */
};

static s32 ext2_is_empty_dir(vfs_inode_t const* node)
{
    if (!node || !S_ISDIR(node->i_mode)) {
        return -ENOTDIR;
    }

    vfs_superblock_t* sb = node->i_sb;
    ext2_super_t* es = sb->u.ext2_sb.s_es;
    u32 pos = 0;

    while (pos < node->i_size) {
        unsigned long offset = pos & (sb->s_blocksize - 1);
        unsigned long block = (pos) >> (es->s_log_block_size + 10);

        if (block >= 12) {
            return -EIO;
        }

        u32 block_num = node->u.i_ext2->i_block[block];
        u8 buff[sb->s_blocksize];

        if (ext2_read_block(node, buff, block_num) < 0) {
            return -EIO;
        }

        while (offset < sb->s_blocksize && pos < node->i_size) {
            ext2_entry_t* entry = (ext2_entry_t*)&buff[offset];

            if (entry->rec_len == 0 || entry->rec_len > sb->s_blocksize) {
                return -EIO;
            }

            if (entry->inode) {
                if (entry->name_len > 2) {
                    return -ENOTEMPTY;
                }

                if (entry->name[0] != '.') {
                    return -ENOTEMPTY;
                }

                if (entry->name_len == 2 && entry->name[1] != '.') {
                    return -ENOTEMPTY;
                }
            }

            offset += entry->rec_len;
            pos += entry->rec_len;
        }
    }

    return 0;
}

s32 ext2_readdir(vfs_inode_t* inode, file_t* file, dirent_t* dirent, s32 count)
{
    (void)count;

    if (!inode || !S_ISDIR(inode->i_mode)) {
        return -1;
    }

    ext2_entry_t* entry;
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
    int err;

    if (!dir || !S_ISDIR(dir->i_mode)) {
        return -1;
    }

    ext2_entry_t* existing = NULL;
    if (ext2_find_entry(dir, name, len, &existing) >= 0) {
        kfree(existing);
        return -1;
    }

    struct vfs_inode* new = ext2_new_inode(dir, mode, &err);
    if (err < 0) {
        return err;
    }

    s32 block_num = ext2_new_block(dir, &err);
    if (err < 0) {
        return err;
    }

    vfs_superblock_t* sb = dir->i_sb;
    new->i_size = sb->s_blocksize;
    new->i_links_count = 2;

    ext2_inode_t* ext2_node = new->u.i_ext2;
    ext2_node->i_size = sb->s_blocksize;
    ext2_node->i_blocks = sb->s_blocksize / 512;
    ext2_node->i_block[0] = block_num;
    for (int i = 1; i < 15; i += 1) {
        ext2_node->i_block[i] = 0;
    }

    ext2_entry_t tmp = { 0 };
    tmp.inode = new->i_ino;
    tmp.name_len = len;
    tmp.file_type = 0;
    memcpy(tmp.name, name, len);

    tmp.rec_len = ALIGN(sizeof(ext2_entry_t) + tmp.name_len, 4);
    if (ext2_write_entry(dir, &tmp) < 0) {
        inode_put(new);
        return -1;
    }

    if (sb->s_op->write_inode(new) < 0) {
        inode_put(new);
        return -1;
    }

    u8 buff[sb->s_blocksize];
    memset(buff, 0, sb->s_blocksize);

    char const* current_str = ".";
    ext2_entry_t current_entry = { 0 };
    current_entry.inode = new->i_ino;
    current_entry.name_len = 1;
    current_entry.file_type = 0;
    memcpy(current_entry.name, current_str, current_entry.name_len);

    u32 rec_len = sizeof(ext2_entry_t) + current_entry.name_len;
    current_entry.rec_len = ALIGN(rec_len, 4);
    memcpy(buff, &current_entry, sizeof(ext2_entry_t) + current_entry.name_len);

    char const* parent_str = "..";
    ext2_entry_t parent_entry = { 0 };
    parent_entry.inode = dir->i_ino;
    parent_entry.name_len = 2;
    parent_entry.file_type = 0;
    memcpy(parent_entry.name, parent_str, parent_entry.name_len);

    parent_entry.rec_len = sb->s_blocksize - current_entry.rec_len;
    memcpy(
        buff + current_entry.rec_len, &parent_entry,
        sizeof(ext2_entry_t) + parent_entry.name_len
    );

    block_device_t* d = get_device(sb->s_dev);
    u32 const addr = ext2_node->i_block[0] * sb->s_blocksize;
    u32 const sector_pos = addr / d->d_sector_size;
    u32 const count = sb->s_blocksize / d->d_sector_size;

    if (d->d_op->write(d, sector_pos, count, buff, sb->s_blocksize) < 0) {
        return -1;
    }

    if (sb->s_op->write_inode(new) < 0) {
        return -1;
    }

    time_t now = getepoch();
    dir->i_links_count += 1;
    dir->i_ctime = now;
    dir->i_mtime = now;

    if (sb->s_op->write_inode(dir) < 0) {
        return -1;
    }

    inode_put(new);

    return 0;
}

int ext2_rmdir(vfs_inode_t* dir, char const* name, int len)
{
    if (!dir) {
        return -ENOENT;
    }

    int retval = 0;
    vfs_inode_t* node = NULL;
    ext2_entry_t* entry = NULL;
    retval = ext2_find_entry(dir, name, len, &entry);
    if (retval < 0) {
        goto end_rmdir;
    }

    retval = -EPERM;
    node = inode_get(dir->i_sb, entry->inode);
    if (!node) {
        goto end_rmdir;
    }

    if (node->i_dev != dir->i_dev) {
        goto end_rmdir;
    }

    if (dir == node) {
        goto end_rmdir;
    }

    if (!S_ISDIR(node->i_mode)) {
        retval = -ENOTDIR;
        goto end_rmdir;
    }

    if (ext2_is_empty_dir(node) < 0) {
        retval = -ENOTEMPTY;
        goto end_rmdir;
    }

    if (node->i_count > 1) {
        node->i_size = 0;
    }

    retval = ext2_delete_entry(dir, entry);
    if (retval < 0) {
        goto end_rmdir;
    }

    retval = ext2_free_inode(node);
    if (retval < 0) {
        goto end_rmdir;
    }

    for (int i = 0; i < 12 && node->u.i_ext2->i_block[i]; i += 1) {
        u32 block = node->u.i_ext2->i_block[i];

        retval = ext2_free_block(node, block); // Keep going, even on error
    }

    time_t now = getepoch();
    dir->i_links_count -= 1;
    dir->i_mtime = dir->i_ctime = now;
    retval = node->i_sb->s_op->write_inode(dir);
    if (retval < 0) {
        printk("%s: Warning: failed to update parent inode\n", __func__);
    }

    node->u.i_ext2->i_dtime = now;
    retval = node->i_sb->s_op->write_inode(node);
    if (retval < 0) {
        printk("%s: Warning: failed to set deletion time\n", __func__);
    }

end_rmdir:
    kfree(entry);
    inode_put(dir);
    inode_put(node);

    return retval;
}
