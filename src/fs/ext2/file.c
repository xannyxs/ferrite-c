#include "sys/file/file.h"
#include "arch/x86/time/time.h"
#include "drivers/block/device.h"
#include "drivers/printk.h"
#include "fs/ext2/ext2.h"
#include "fs/stat.h"
#include "fs/vfs.h"
#include "lib/math.h"
#include "sys/file/fcntl.h"

#include <ferrite/errno.h>
#include <ferrite/types.h>
#include <lib/string.h>

static int ext2_file_read(struct vfs_inode*, struct file*, void*, int);
static int ext2_file_write(struct vfs_inode*, struct file*, void const*, int);
// static void ext2_release_file(struct vfs_inode*, struct file*);

static struct file_operations ext2_file_operations = {
    .read = ext2_file_read,
    .write = ext2_file_write,
    .readdir = NULL,
    .lseek = NULL,

    // .release = ext2_release_file,
};

struct inode_operations ext2_file_inode_operations = {
    .default_file_ops = &ext2_file_operations,
    .create = NULL,
    .lookup = NULL,
    .mkdir = NULL,
    .unlink = NULL,
    .truncate = ext2_truncate,
    .permission = ext2_permission,
};

int ext2_file_read(vfs_inode_t* node, file_t* file, void* buff, s32 count)
{
    if (!node) {
        return -1;
    }

    if ((file->f_flags & 0x03) == O_WRONLY) {
        return -EBADF;
    }

    block_device_t* d = get_device(node->i_dev);
    if (!d || !d->d_op || !d->d_op->read) {
        printk("%s: device has no read operation\n", __func__);
        return -1;
    }

    vfs_superblock_t* sb = node->i_sb;
    ext2_inode_t* ext2_node = node->u.i_ext2;
    if (!S_ISREG(node->i_mode)) {
        printk("%s: not a regular file\n", __func__);
        return -EINVAL;
    }

    u32 const sectors_per_block = sb->s_blocksize / d->d_sector_size;
    u32 offset = file->f_pos;
    s32 bytes_copied = 0;
    u8 block_buffer[sb->s_blocksize];

    while (bytes_copied < count && offset < ext2_node->i_size) {
        u32 i = offset / sb->s_blocksize;
        if (i >= 12) {
            break;
        }

        u32 offset_in_block = offset % sb->s_blocksize;
        s32 bytes_in_this_block = (s32)sb->s_blocksize - offset_in_block;
        s32 bytes_remaining = (s32)ext2_node->i_size - offset;
        s32 bytes_to_copy = min(bytes_in_this_block, count - bytes_copied);
        bytes_to_copy = min(bytes_to_copy, bytes_remaining);

        u32 addr = ext2_node->i_block[i] * sb->s_blocksize;
        u32 sector_pos = addr / d->d_sector_size;

        if (d->d_op->read(
                d, sector_pos, sectors_per_block, block_buffer, sb->s_blocksize
            )
            < 0) {
            printk(
                "%s: failed to read from device (LBA %u, "
                "count %u)\n",
                __func__, sector_pos, 1
            );
            return -1;
        }

        memcpy(
            (u8*)buff + bytes_copied, block_buffer + offset_in_block,
            bytes_to_copy
        );
        bytes_copied += bytes_to_copy;
        offset += bytes_to_copy;
    }

    file->f_pos = offset;
    return (s32)bytes_copied;
}

int ext2_file_write(
    struct vfs_inode* dir,
    struct file* file,
    void const* buff,
    int count
)
{
    if (!dir) {
        return -1;
    }

    if ((file->f_flags & 0x03) == O_RDONLY) {
        return -EBADF;
    }

    if (!S_ISREG(dir->i_mode)) {
        printk("%s: not a regular file\n", __func__);
        return -1;
    }

    ext2_inode_t* node = dir->u.i_ext2;
    vfs_superblock_t* sb = dir->i_sb;

    s32 offset = file->f_pos;
    u32 bytes_written = 0;
    u32 start_block = offset / sb->s_blocksize;
    u32 end_block = (offset + count - 1) / sb->s_blocksize;

    for (u32 i = start_block; i <= end_block; i += 1) {
        if (i >= 12) {
            return bytes_written > 0 ? (s32)bytes_written : -EFBIG;
        }

        s32 block_num;
        s32 err = 0;

        if (node->i_block[i] == 0) {
            block_num = ext2_new_block(dir, &err);
            if (err) {
                return err;
            }
            node->i_block[i] = block_num;
        } else {
            block_num = (s32)node->i_block[i];
        }

        u32 block_offset = (i == start_block) ? (offset % sb->s_blocksize) : 0;
        u32 bytes_to_write = sb->s_blocksize - block_offset;
        if (bytes_written + bytes_to_write > (u32)count) {
            bytes_to_write = count - bytes_written;
        }

        if (ext2_write_block(
                dir, block_num, (u8 const*)buff + bytes_written, block_offset,
                sb->s_blocksize
            )
            < 0) {
            return bytes_written > 0 ? (s32)bytes_written : -1;
        }
        bytes_written += bytes_to_write;
    }

    u32 now = getepoch();
    node->i_ctime = now;
    node->i_mtime = now;
    dir->i_ctime = now;
    dir->i_mtime = now;

    if (offset + count > (s32)node->i_size) {
        node->i_size = offset + count;
        dir->i_size = offset + count;
    }

    u32 allocated_blocks = 0;
    for (u32 i = 0; i < 12; i++) {
        if (node->i_block[i] != 0) {
            allocated_blocks++;
        }
    }
    node->i_blocks = (allocated_blocks * sb->s_blocksize) / 512;
    if (sb->s_op->write_inode(dir) < 0) {
        return -1;
    }

    file->f_pos = offset + bytes_written;
    return (s32)bytes_written;
}
