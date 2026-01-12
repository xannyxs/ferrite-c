#include "drivers/block/device.h"
#include "drivers/printk.h"
#include "fs/ext2/ext2.h"
#include "fs/stat.h"
#include "fs/vfs.h"
#include "memory/kmalloc.h"

#include <ferrite/errno.h>
#include <ferrite/string.h>
#include <ferrite/types.h>

extern struct inode_operations chrdev_inode_ops;

static s32 ext2_read_inode(vfs_inode_t*);
static s32 ext2_write_inode(vfs_inode_t*);
// static void ext2_put_inode(vfs_inode_t*);

struct super_operations ext2_sops = {
    .read_inode = ext2_read_inode,
    .write_inode = ext2_write_inode,
    .write_super = ext2_superblock_write,

    // .put_inode = ext2_put_inode,
    // .put_super = ext_put_super,
};

s32 ext2_write_inode(vfs_inode_t* dir)
{
    if (!dir || !dir->i_sb) {
        printk("ext2_write_inode: invalid inode\n");
        return -EINVAL;
    }

    vfs_superblock_t* sb = dir->i_sb;
    block_device_t* d = get_device(dir->i_sb->s_dev);
    if (!d || !d->d_op || !d->d_op->write) {
        printk("%s: device has no write operation\n", __func__);
        return -1;
    }

    ext2_super_t* es = sb->u.ext2_sb.s_es;
    ext2_inode_t* ext2_inode = dir->u.i_ext2;

    u32 block_group = (dir->i_ino - 1) / es->s_inodes_per_group;
    u32 local_index = (dir->i_ino - 1) % es->s_inodes_per_group;
    ext2_block_group_descriptor_t* bgd
        = &sb->u.ext2_sb.s_group_desc[block_group];

    u32 inode_table_offset = local_index * es->s_inode_size;
    u32 inode_table_start_byte = bgd->bg_inode_table * sb->s_blocksize;
    u32 absolute_addr = inode_table_start_byte + inode_table_offset;

    u32 sector_pos = absolute_addr / d->d_sector_size;
    u32 offset_in_sector = absolute_addr % d->d_sector_size;

    u8 buff[d->d_sector_size];
    if (d->d_op->read(d, sector_pos, 1, buff, d->d_sector_size) < 0) {
        return -1;
    }

    ext2_inode_t* disk_inode = (ext2_inode_t*)(buff + offset_in_sector);

    if (S_ISCHR(dir->i_mode) || S_ISBLK(dir->i_mode)) {
        ext2_inode->i_block[0] = dir->i_rdev;
    }

    ext2_inode->i_mode = dir->i_mode;
    ext2_inode->i_size = dir->i_size;
    ext2_inode->i_uid = dir->i_uid;
    ext2_inode->i_gid = dir->i_gid;
    ext2_inode->i_atime = dir->i_atime;
    ext2_inode->i_mtime = dir->i_mtime;
    ext2_inode->i_ctime = dir->i_ctime;
    ext2_inode->i_dtime = 0;
    ext2_inode->i_links_count = dir->i_links_count;

    memcpy(disk_inode, ext2_inode, es->s_inode_size);

    if (d->d_op->write(d, sector_pos, 1, buff, d->d_sector_size) < 0) {
        return -1;
    }

    return 0;
}

int ext2_read_inode(vfs_inode_t* dir)
{
    block_device_t* d = get_device(dir->i_sb->s_dev);
    if (!d || !d->d_op || !d->d_op->read) {
        printk("%s: device has no read operation\n", __func__);
        return -1;
    }

    if (!dir->u.i_ext2) {
        dir->u.i_ext2 = kmalloc(sizeof(ext2_inode_t));
        if (!dir->u.i_ext2) {
            return -1;
        }
    }
    ext2_inode_t* node = dir->u.i_ext2;
    vfs_superblock_t* sb = dir->i_sb;

    u32 block_group = (dir->i_ino - 1) / sb->u.ext2_sb.s_es->s_inodes_per_group;
    ext2_block_group_descriptor_t* bgd
        = &sb->u.ext2_sb.s_group_desc[block_group];

    u32 index = (dir->i_ino - 1) % sb->u.ext2_sb.s_es->s_inodes_per_group;
    u32 offset_in_table = index * sb->u.ext2_sb.s_es->s_inode_size;
    u32 addr = (bgd->bg_inode_table * sb->s_blocksize) + offset_in_table;
    u32 sector_pos = addr / d->d_sector_size;

    u8 buff[d->d_sector_size];
    if (d->d_op->read(d, sector_pos, 1, buff, d->d_sector_size) < 0) {
        return -1;
    }

    u32 offset = addr % d->d_sector_size;
    if (offset > d->d_sector_size) {
        printk("%s: offset it bigger than sector size\n", __func__);
        return -1;
    }

    memcpy(node, &buff[offset], sizeof(ext2_inode_t));

    if (S_ISCHR(node->i_mode) || S_ISBLK(node->i_mode)) {
        dir->i_rdev = node->i_block[0];
    }

    dir->i_dev = sb->s_dev;
    dir->i_atime = (time_t)node->i_atime;
    dir->i_mtime = (time_t)node->i_mtime;
    dir->i_ctime = (time_t)node->i_ctime;
    dir->i_links_count = node->i_links_count;
    dir->i_mode = node->i_mode;
    dir->i_size = node->i_size;

    dir->i_uid = node->i_uid;
    dir->i_gid = node->i_gid;

    if (S_ISDIR(dir->i_mode)) {
        dir->i_op = &ext2_dir_inode_operations;
    } else if (S_ISREG(dir->i_mode)) {
        dir->i_op = &ext2_file_inode_operations;
    } else if (S_ISCHR(dir->i_mode)) {
        dir->i_op = &chrdev_inode_ops;
    } else {
        printk("Warning: no operation for inode %d\n", dir->i_ino);
    }

    return 0;
}
