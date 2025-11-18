#include "drivers/block/device.h"
#include "drivers/printk.h"
#include "fs/ext2/ext2.h"
#include "fs/vfs.h"
#include "lib/math.h"
#include "memory/kmalloc.h"
#include "sys/file/stat.h"
#include <ferrite/types.h>

#include <lib/string.h>
#include <stdbool.h>

extern struct inode_operations ext2_dir_inode_ops;

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

int mark_inode_bitmap(vfs_inode_t* node, bool allocate)
{
    vfs_superblock_t* sb = node->i_sb;
    ext2_super_t* es = sb->u.ext2_sb.s_es;

    u32 bgd_index = (node->i_ino - 1) / es->s_inodes_per_group;
    ext2_block_group_descriptor_t* bgd = &sb->u.ext2_sb.s_group_desc[bgd_index];

    u8 bitmap[sb->s_blocksize];
    if (ext2_read_block(node, bitmap, bgd->bg_inode_bitmap) < 0) {
        return -1;
    }

    u32 local_inode_index = (node->i_ino - 1) % es->s_inodes_per_group;
    u32 byte_index = local_inode_index / 8;
    u32 bit_index = local_inode_index % 8;

    if (allocate) {
        if (bitmap[byte_index] & (1 << bit_index)) {
            printk(
                "%s: Warning: inode %u already allocated\n", __func__,
                node->i_ino
            );
            return -1;
        }

        bitmap[byte_index] |= (1 << bit_index);
        bgd->bg_free_inodes_count--;
        es->s_free_inodes_count--;
    } else {
        if (!(bitmap[byte_index] & (1 << bit_index))) {
            printk(
                "%s: Warning: inode %u already freed\n", __func__, node->i_ino
            );
            return -1;
        }

        bitmap[byte_index] &= ~(1 << bit_index);
        bgd->bg_free_inodes_count++;
        es->s_free_inodes_count++;
    }

    block_device_t* d = get_device(sb->s_dev);
    if (!d) {
        return -1;
    }

    u32 sectors_per_block = sb->s_blocksize / d->d_sector_size;
    u32 sector_num = bgd->bg_inode_bitmap * sectors_per_block;
    if (d->d_op->write(
            d, sector_num, sectors_per_block, bitmap, sb->s_blocksize
        )
        < 0) {
        return -1;
    }

    // if (ext2_block_group_descriptors_write(m, bgd_index) < 0) {
    //     return -1;
    // }

    if (ext2_superblock_write(sb) < 0) {
        return -1;
    }

    return 0;
}

inline int mark_inode_allocated(vfs_inode_t* node)
{
    return mark_inode_bitmap(node, true);
}

inline int mark_inode_free(vfs_inode_t* node)
{
    return mark_inode_bitmap(node, false);
}

int find_free_inode(vfs_inode_t* node)
{
    vfs_superblock_t* sb = node->i_sb;
    ext2_super_t* es = sb->u.ext2_sb.s_es;
    ext2_block_group_descriptor_t* bgd = NULL;

    u32 i;
    u32 bgd_count = CEIL_DIV(es->s_inodes_count, es->s_inodes_per_group);
    for (i = 0; i < bgd_count; i += 1) {
        if (sb->u.ext2_sb.s_group_desc[i].bg_free_inodes_count != 0) {
            bgd = &sb->u.ext2_sb.s_group_desc[i];
            break;
        }
    }

    if (!bgd) {
        printk("%s: No free inode\n", __func__);
        return -1;
    }

    u8 bitmap[sb->s_blocksize];
    ext2_read_block(node, bitmap, bgd->bg_inode_bitmap); // Error check?

    int bit = find_free_bit_in_bitmap(bitmap, sb->s_blocksize);
    if (bit < 0) {
        return -1;
    }

    return (s32)(bit + (es->s_inodes_per_group * i) + 1);
}

s32 ext2_write_inode(vfs_inode_t* dir)
{
    block_device_t* d = get_device(dir->i_sb->s_dev);
    if (!d) {
        printk("%s: device is NULL\n", __func__);
        return -1;
    }

    if (!d->d_op || !d->d_op->write) {
        printk("%s: device has no write operation\n", __func__);
        return -1;
    }

    vfs_superblock_t* sb = dir->i_sb;
    struct ext2_superblock* es = sb->u.ext2_sb.s_es;
    ext2_inode_t* node = dir->u.i_ext2;

    u32 block_group = (dir->i_ino - 1) / es->s_inodes_per_group;
    struct ext2_block_group_descriptor* bgd
        = &sb->u.ext2_sb.s_group_desc[block_group];

    u32 local_inode_index = (dir->i_ino - 1) % es->s_inodes_per_group;
    u32 inode_table_offset = local_inode_index * es->s_inode_size;

    u32 inode_table_start_byte = bgd->bg_inode_table * sb->s_blocksize;
    u32 absolute_addr = inode_table_start_byte + inode_table_offset;

    u32 sector_pos = absolute_addr / d->d_sector_size;
    u32 offset_in_sector = absolute_addr % d->d_sector_size;

    u8 buff[d->d_sector_size];
    if (d->d_op->read(d, sector_pos, 1, buff, d->d_sector_size) < 0) {
        return -1;
    }

    node->i_mode = dir->i_mode;
    node->i_size = dir->i_size;
    node->i_uid = dir->i_uid;
    node->i_gid = dir->i_gid;
    node->i_atime = dir->i_atime;
    node->i_mtime = dir->i_mtime;
    node->i_ctime = dir->i_ctime;
    node->i_links_count = dir->i_links_count;

    memcpy(&buff[offset_in_sector], node, es->s_inode_size);

    if (d->d_op->write(d, sector_pos, 1, buff, d->d_sector_size) < 0) {
        return -1;
    }

    return 0;
}

s32 ext2_read_inode(vfs_inode_t* dir)
{
    block_device_t* d = get_device(dir->i_sb->s_dev);
    if (!d) {
        printk("%s: device is NULL\n", __func__);
        return -1;
    }

    if (!d->d_op || !d->d_op->read) {
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
        printk(
            "%s: failed to read from device (LBA %u, "
            "count %u)\n",
            __func__, sector_pos, 1
        );
        return -1;
    }

    u32 offset = addr % d->d_sector_size;
    if (offset > d->d_sector_size) {
        printk("%s: offset it bigger than sector size\n", __func__);
        return -1;
    }
    memcpy(node, &buff[offset], sizeof(ext2_inode_t));

    dir->i_atime = (time_t)node->i_atime;
    dir->i_mtime = (time_t)node->i_mtime;
    dir->i_ctime = (time_t)node->i_ctime;
    dir->i_links_count = node->i_links_count;

    dir->i_uid = node->i_uid;
    dir->i_gid = node->i_gid;

    if (S_ISDIR(dir->i_mode)) {
        dir->i_op = &ext2_dir_inode_ops;
    } else {
        // dir->i_op = &ext2_file_inode_ops; // TODO
        return -1;
    }

    return 0;
}
