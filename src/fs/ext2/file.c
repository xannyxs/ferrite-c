#include "sys/file/file.h"
#include "arch/x86/time/time.h"
#include "drivers/printk.h"
#include "fs/vfs.h"
#include "lib/math.h"
#include "sys/file/stat.h"

#include <ferrite/types.h>
#include <lib/string.h>

extern ext2_mount_t ext2_mounts[MAX_EXT2_MOUNTS];

// static int ext2_file_read(struct vfs_inode*, struct file*, void*, int);
// static int ext2_file_write(struct vfs_inode*, struct file*, void const*,
// int); static void ext2_release_file(struct vfs_inode*, struct file*);
//
// static struct file_operations ext2_file_operations = {
//     .read = ext2_file_read,
//     .write = ext2_file_write,
//     .readdir = NULL,
//     .release = ext2_release_file,
// };
//
// struct inode_operations ext2_file_inode_operations = {
//     .default_file_ops = &ext2_file_operations,
//     .create = NULL,
//     .lookup = NULL,
//     .mkdir = NULL,
// };

// int ext2_read(struct vfs_inode* vfs, struct file* file, void* buff, s32
// count)
// {
//     ext2_mount_t* m = &ext2_mounts[0];
//     block_device_t* d = m->m_device;
//     if (!d) {
//         printk("%s: device is NULL\n", __func__);
//         return -1;
//     }
//
//     if (!d->d_op || !d->d_op->read) {
//         printk("%s: device has no read operation\n", __func__);
//         return -1;
//     }
//
//     if (!vfs || !vfs->u.i_ext2) {
//         return -1;
//     }
//     ext2_inode_t* inode = (ext2_inode_t*)vfs->u.i_ext2;
//     if (S_ISREG(inode->i_mode)) {
//         printk("%s: not a regular file\n", __func__);
//         return -1;
//     }
//
//     ext2_super_t s = m->m_superblock;
//     u32 block_size = 1024 << s.s_log_block_size;
//
//     u32 current_file_offset = offset;
//     u32 bytes_copied = 0;
//     u8 block_buffer[block_size];
//
//     while (bytes_copied < len && current_file_offset < inode->i_size) {
//         u32 i = current_file_offset / block_size;
//         u32 offset_in_block = current_file_offset % block_size;
//         u32 bytes_in_this_block = block_size - offset_in_block;
//         u32 bytes_to_copy = min(bytes_in_this_block, len - bytes_copied);
//
//         u32 addr = inode->i_block[i] * block_size;
//         u32 sector_pos = addr / d->sector_size;
//
//         if (d->d_op->read(
//                 d, sector_pos, inode->i_blocks, block_buffer, block_size
//             )
//             < 0) {
//             printk(
//                 "%s: failed to read from device (LBA %u, "
//                 "count %u)\n",
//                 __func__, sector_pos, 1
//             );
//             return -1;
//         }
//
//         memcpy(
//             (u8*)buff + bytes_copied, block_buffer + offset_in_block,
//             bytes_to_copy
//         );
//         bytes_copied += bytes_to_copy;
//         current_file_offset += bytes_to_copy;
//     }
//
//     return (s32)bytes_copied;
// }
//
// int ext2_file_write(
//     struct vfs_inode* vfs,
//     struct file* file,
//     void const* buff,
//     int count
// )
// {
//     if (!vfs || !vfs->u.i_ext2) {
//         return -1;
//     }
//
//     ext2_inode_t* node = vfs->u.i_ext2;
//     ext2_mount_t* m = &ext2_mounts[0];
//
//     // TODO: Support double / triple pointers
//     if (len > m->m_block_size * 13) {
//         printk("We currently do not support double or triple pointers\n");
//         return -1;
//     }
//
//     if (S_ISREG(vfs->i_mode) == false) {
//         printk("%s: not a regular file\n", __func__);
//         return -1;
//     }
//
//     u32 bytes_written = 0;
//     u32 start_block = offset / m->m_block_size;
//     u32 end_block = (offset + len - 1) / m->m_block_size;
//     for (u32 i = start_block; i <= end_block; i++) {
//         s32 block_num;
//
//         if (node->i_block[i] == 0) {
//             // Should the blocks be continious?
//             block_num = find_free_block(m);
//             if (block_num < 0) {
//                 // TODO: what if it is stops mid-loop.
//                 printk("%s: Could not find free block\n", __func__);
//                 return bytes_written > 0 ? (s32)bytes_written : -1;
//             }
//
//             if (mark_block_allocated(m, block_num) < 0) {
//                 return bytes_written > 0 ? (s32)bytes_written : -1;
//             }
//         } else {
//             block_num = (s32)node->i_block[i];
//         }
//
//         u32 block_offset = (i == start_block) ? (offset % m->m_block_size) :
//         0; u32 bytes_to_write = m->m_block_size - block_offset;
//
//         if (bytes_written + bytes_to_write > len) {
//             bytes_to_write = len - bytes_written;
//         }
//
//         if (ext2_write_block(
//                 m, block_num, (u8 const*)buff + bytes_written, block_offset,
//                 m->m_block_size
//             )
//             < 0) {
//             return bytes_written > 0 ? (s32)bytes_written : -1;
//         }
//         bytes_written += bytes_to_write;
//     }
//
//     u32 now = getepoch();
//     node->i_ctime = now;
//     node->i_mtime = now;
//
//     if (offset + len > node->i_size) {
//         node->i_size = offset + len;
//     }
//     u32 allocated_blocks = 0;
//     for (u32 i = 0; i < 13; i += 1) {
//         if (node->i_block[i] != 0) {
//             allocated_blocks += 1;
//         }
//     }
//     node->i_blocks = (allocated_blocks * m->m_block_size) / 512;
//
//     if (ext2_write_inode(m, vfs->i_ino, node) < 0) {
//         return -1;
//     }
//
//     return (s32)bytes_written;
// }
