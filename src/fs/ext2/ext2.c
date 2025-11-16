#include "fs/ext2/ext2.h"
#include "arch/x86/time/time.h"
#include "drivers/block/device.h"
#include "drivers/printk.h"
#include "fs/vfs.h"
#include "fs/vfs/mode_t.h"
#include "lib/math.h"
#include "lib/stdlib.h"
#include "lib/string.h"
#include "memory/kmalloc.h"
#include "sys/file/stat.h"
#include <ferrite/types.h>

// TODO:
// - ext2_rename (for dirs)
// - ext2_directory_rename - create directory if it does not exist

ext2_mount_t ext2_mounts[MAX_EXT2_MOUNTS];

static vfs_inode_t* ext2_lookup(vfs_inode_t* inode, char const* name);
static vfs_inode_t* ext2_inode_get(u32 inode_num);
static void ext2_inode_put(vfs_inode_t* inode);

static int ext2_read(vfs_inode_t*, void* buff, u32 offset, u32 len);
static int ext2_write(vfs_inode_t* vfs, void const* buff, u32 offset, u32 len);
static int ext2_create(vfs_inode_t* parent, vfs_dentry_t* dentry, mode_t mode);
static int ext2_mkdir(vfs_inode_t* parent, vfs_dentry_t* dentry, mode_t mode);

static struct inode_operations ext2_fops = {
    .lookup = ext2_lookup,
    .inode_get = ext2_inode_get,
    .inode_put = ext2_inode_put,

    .read = ext2_read,
    .write = ext2_write,
    .create = ext2_create,
    .mkdir = ext2_mkdir,
};

inline int find_free_bit_in_bitmap(u8 const* bitmap, u32 size)
{
    for (u32 i = 0; i < size; i += 1) {
        u8 byte = bitmap[i];

        if (byte == 0xFF) {
            continue;
        }

        for (int j = 0; j < 8; j += 1) {
            if (!(byte & (1 << j))) {
                return (s32)(i * 8 + j);
            }
        }
    }

    return -1;
}

int ext2_write(vfs_inode_t* vfs, void const* buff, u32 offset, u32 len)
{
    if (!vfs || !vfs->u.i_ext2) {
        return -1;
    }

    ext2_inode_t* node = vfs->u.i_ext2;
    ext2_mount_t* m = &ext2_mounts[0];

    // TODO: Support double / triple pointers
    if (len > m->m_block_size * 13) {
        printk("We currently do not support double or triple pointers\n");
        return -1;
    }

    if (S_ISREG(vfs->i_mode) == false) {
        printk("%s: not a regular file\n", __func__);
        return -1;
    }

    u32 bytes_written = 0;
    u32 start_block = offset / m->m_block_size;
    u32 end_block = (offset + len - 1) / m->m_block_size;
    for (u32 i = start_block; i <= end_block; i++) {
        s32 block_num;

        if (node->i_block[i] == 0) {
            // Should the blocks be continious?
            block_num = find_free_block(m);
            if (block_num < 0) {
                // TODO: what if it is stops mid-loop.
                printk("%s: Could not find free block\n", __func__);
                return bytes_written > 0 ? (s32)bytes_written : -1;
            }

            if (mark_block_allocated(m, block_num) < 0) {
                return bytes_written > 0 ? (s32)bytes_written : -1;
            }
        } else {
            block_num = (s32)node->i_block[i];
        }

        u32 block_offset = (i == start_block) ? (offset % m->m_block_size) : 0;
        u32 bytes_to_write = m->m_block_size - block_offset;

        if (bytes_written + bytes_to_write > len) {
            bytes_to_write = len - bytes_written;
        }

        if (ext2_write_block(
                m, block_num, (u8 const*)buff + bytes_written, block_offset,
                m->m_block_size
            )
            < 0) {
            return bytes_written > 0 ? (s32)bytes_written : -1;
        }
        bytes_written += bytes_to_write;
    }

    u32 now = getepoch();
    node->i_ctime = now;
    node->i_mtime = now;

    if (offset + len > node->i_size) {
        node->i_size = offset + len;
    }
    u32 allocated_blocks = 0;
    for (u32 i = 0; i < 13; i += 1) {
        if (node->i_block[i] != 0) {
            allocated_blocks += 1;
        }
    }
    node->i_blocks = (allocated_blocks * m->m_block_size) / 512;

    if (ext2_write_inode(m, vfs->i_ino, node) < 0) {
        return -1;
    }

    return (s32)bytes_written;
}

int ext2_mkdir(vfs_inode_t* parent, vfs_dentry_t* dentry, mode_t mode)
{
    if (!parent) {
        return -1;
    }

    if (S_ISDIR(mode) == false) {
        return -1;
    }

    ext2_mount_t* m = &ext2_mounts[0];

    s32 node_num = find_free_inode(m);
    if (node_num < 0) {
        return -1;
    }

    if (mark_inode_allocated(m, node_num) < 0) {
        return -1;
    }

    s32 block_num = find_free_block(m);
    if (block_num < 0) {
        return -1;
    }

    if (mark_block_allocated(m, block_num) < 0) {
        return -1;
    }

    vfs_inode_t* new = kmalloc(sizeof(vfs_inode_t));
    if (!new) {
        return -1;
    }

    ext2_inode_t* node = kmalloc(sizeof(ext2_inode_t));
    if (!node) {
        kfree(new);
        return -1;
    }

    node->i_mode = mode;
    node->i_uid = 0;

    node->i_size = m->m_block_size;
    node->i_blocks = (1 * m->m_block_size) / 512;
    node->i_block[0] = block_num;
    for (int i = 1; i < 15; i += 1) {
        node->i_block[i] = 0;
    }

    u32 now = getepoch();
    node->i_atime = now;
    node->i_ctime = now;
    node->i_mtime = now;
    node->i_dtime = 0;

    node->i_gid = 0;
    node->i_links_count = 2;
    node->i_flags = 0;

    new->i_ino = node_num;
    new->i_mode = mode;
    new->i_size = m->m_block_size;
    new->i_op = &ext2_fops;
    new->u.i_ext2 = node;

    ext2_entry_t entry = { 0 };
    entry.inode = node_num;
    entry.name_len = strlen(dentry->d_name);
    entry.file_type = 0;
    memcpy(entry.name, dentry->d_name, entry.name_len);

    entry.rec_len = sizeof(ext2_entry_t) + entry.name_len;
    if (ext2_entry_write(m, &entry, parent->i_ino) < 0) {
        kfree(node);
        kfree(new);
        return -1;
    }

    if (ext2_write_inode(m, node_num, node) < 0) {
        kfree(node);
        kfree(new);
        return -1;
    }

    u8 buff[m->m_block_size];

    char const* current_str = ".";
    ext2_entry_t current_entry = { 0 };
    current_entry.inode = node_num;
    current_entry.name_len = strlen(current_str);
    current_entry.file_type = 0;
    memcpy(current_entry.name, current_str, current_entry.name_len);

    current_entry.rec_len
        = ALIGN(sizeof(ext2_entry_t) + current_entry.name_len, 4);
    memcpy(buff, &current_entry, current_entry.rec_len);

    char const* parent_str = "..";
    ext2_entry_t parent_entry = { 0 };
    parent_entry.inode = parent->i_ino;
    parent_entry.name_len = strlen(parent_str);
    parent_entry.file_type = 0;
    memcpy(parent_entry.name, parent_str, parent_entry.name_len);

    parent_entry.rec_len = m->m_block_size - current_entry.rec_len;
    memcpy(buff + current_entry.rec_len, &parent_entry, parent_entry.rec_len);

    block_device_t* d = m->m_device;
    u32 const addr = node->i_block[0] * m->m_block_size;
    u32 sector_pos = addr / d->sector_size;
    u32 const count = m->m_block_size / d->sector_size;

    if (d->d_op->write(d, sector_pos, count, buff, m->m_block_size) < 0) {
        return -1;
    }

    ext2_inode_t parent_inode = { 0 };
    if (ext2_read_inode(m, parent->i_ino, &parent_inode) < 0) {
        kfree(node);
        kfree(new);
        return -1;
    }

    parent_inode.i_links_count += 1;
    parent_inode.i_ctime = now;
    parent_inode.i_mtime = now;

    if (ext2_write_inode(m, parent->i_ino, &parent_inode) < 0) {
        kfree(node);
        kfree(new);
        return -1;
    }

    dentry->d_inode = new;
    return 0;
}

int ext2_create(vfs_inode_t* parent, vfs_dentry_t* dentry, mode_t mode)
{
    if (!parent) {
        return -1;
    }

    ext2_mount_t* m = &ext2_mounts[0];

    s32 node_num = find_free_inode(m);
    if (node_num < 0) {
        return -1;
    }

    if (mark_inode_allocated(m, node_num) < 0) {
        return -1;
    }

    vfs_inode_t* new = kmalloc(sizeof(vfs_inode_t));
    if (!new) {
        return -1;
    }

    ext2_inode_t* node = kmalloc(sizeof(ext2_inode_t));
    if (!node) {
        kfree(new);
        return -1;
    }

    node->i_mode = mode;
    node->i_uid = 0;
    node->i_size = 0;

    u32 now = getepoch();
    node->i_atime = now;
    node->i_ctime = now;
    node->i_mtime = now;
    node->i_dtime = 0;

    node->i_gid = 0;
    node->i_links_count = 1;
    node->i_flags = 0;
    node->i_blocks = 0;
    for (int i = 0; i < 15; i += 1) {
        node->i_block[i] = 0;
    }

    new->i_ino = node_num;
    new->i_mode = mode;
    new->i_size = 0;
    new->i_op = &ext2_fops;
    new->u.i_ext2 = node;

    ext2_entry_t entry = { 0 };
    entry.inode = node_num;
    entry.name_len = strlen(dentry->d_name);
    entry.file_type = 0;
    memcpy(entry.name, dentry->d_name, entry.name_len);

    entry.rec_len = sizeof(ext2_entry_t) + entry.name_len;
    if (ext2_entry_write(m, &entry, parent->i_ino) < 0) {
        kfree(node);
        kfree(new);
        return -1;
    }

    if (ext2_write_inode(m, node_num, node) < 0) {
        kfree(node);
        kfree(new);
        return -1;
    }

    dentry->d_inode = new;
    return 0;
}

int ext2_read(vfs_inode_t* vfs, void* buff, u32 offset, u32 len)
{
    ext2_mount_t* m = &ext2_mounts[0];
    block_device_t* d = m->m_device;
    if (!d) {
        printk("%s: device is NULL\n", __func__);
        return -1;
    }

    if (!d->d_op || !d->d_op->read) {
        printk("%s: device has no read operation\n", __func__);
        return -1;
    }

    if (!vfs || !vfs->u.i_ext2) {
        return -1;
    }
    ext2_inode_t* inode = (ext2_inode_t*)vfs->u.i_ext2;
    if (S_ISREG(inode->i_mode)) {
        printk("%s: not a regular file\n", __func__);
        return -1;
    }

    ext2_super_t s = m->m_superblock;
    u32 block_size = 1024 << s.s_log_block_size;

    u32 current_file_offset = offset;
    u32 bytes_copied = 0;
    u8 block_buffer[block_size];

    while (bytes_copied < len && current_file_offset < inode->i_size) {
        u32 i = current_file_offset / block_size;
        u32 offset_in_block = current_file_offset % block_size;
        u32 bytes_in_this_block = block_size - offset_in_block;
        u32 bytes_to_copy = min(bytes_in_this_block, len - bytes_copied);

        u32 addr = inode->i_block[i] * block_size;
        u32 sector_pos = addr / d->sector_size;

        if (d->d_op->read(
                d, sector_pos, inode->i_blocks, block_buffer, block_size
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
        current_file_offset += bytes_to_copy;
    }

    return (s32)bytes_copied;
}

vfs_inode_t* ext2_inode_get(u32 inode_num)
{
    ext2_mount_t* m = &ext2_mounts[0];

    ext2_inode_t* found_inode = kmalloc(sizeof(ext2_inode_t));
    if (!found_inode) {
        return NULL;
    }

    if (ext2_read_inode(m, inode_num, found_inode) < 0) {
        kfree(found_inode);
        return NULL;
    }

    vfs_inode_t* new = kmalloc(sizeof(vfs_inode_t));
    if (!new) {
        kfree(found_inode);
        return NULL;
    }

    new->i_ino = inode_num;
    new->i_size = found_inode->i_size;
    new->i_mode = found_inode->i_mode;
    new->u.i_ext2 = found_inode;
    new->i_op = &ext2_fops;

    return new;
}

void ext2_inode_put(vfs_inode_t* inode)
{
    if (inode && inode->u.i_ext2) {
        kfree(inode->u.i_ext2);
    }
}

/**
 * ext2_lookup() allocates and needs to be freed
 */
vfs_inode_t* ext2_lookup(vfs_inode_t* inode, char const* name)
{
    ext2_entry_t* entry = NULL;
    ext2_mount_t* m = &ext2_mounts[0];

    if (ext2_read_entry(m, &entry, inode->i_ino, name) < 0) {
        return NULL;
    }

    ext2_inode_t* found_inode = kmalloc(sizeof(ext2_inode_t));
    if (!found_inode) {
        kfree(entry);
        return NULL;
    }

    if (ext2_read_inode(m, entry->inode, found_inode) < 0) {
        kfree(entry);
        kfree(found_inode);
        return NULL;
    }

    vfs_inode_t* new = kmalloc(sizeof(vfs_inode_t));
    if (!new) {
        kfree(entry);
        kfree(found_inode);
        return NULL;
    }

    new->i_ino = entry->inode;
    new->i_size = found_inode->i_size;
    new->i_mode = found_inode->i_mode;
    new->u.i_ext2 = found_inode;
    new->i_op = &ext2_fops;

    kfree(entry);

    return new;
}

vfs_inode_t* ext2_get_root_node(void) { return ext2_inode_get(2); }

s32 ext2_mount(block_device_t* d)
{
    if (!d) {
        return -1;
    }

    ext2_mount_t* m = NULL;

    for (int i = 0; i < MAX_EXT2_MOUNTS; i += 1) {
        if (!ext2_mounts[i].m_device) {
            m = &ext2_mounts[i];
            break;
        }
    }

    if (!m) {
        return -1;
    }

    m->m_device = d;
    if (ext2_superblock_read(m) < 0) {
        return -1;
    }

    m->num_block_groups = CEIL_DIV(
        m->m_superblock.s_blocks_count, m->m_superblock.s_blocks_per_group
    );
    u32 bgd_bytes = m->num_block_groups * sizeof(ext2_block_group_descriptor_t);
    u32 bgd_sectors = CEIL_DIV(bgd_bytes, d->sector_size);

    m->m_block_group = kmalloc(bgd_sectors * d->sector_size);
    if (!m->m_block_group) {
        return -1;
    }

    if (ext2_block_group_descriptors_read_all(
            m, m->m_block_group, m->num_block_groups
        )
        < 0) {
        return -1;
    }

    if (ext2_read_inode(m, 2, &m->m_inode) < 0) {
        return -1;
    }

    return 0;
}

void ext2_umount(void) { }

// TODO:
// Should I already mount
// get_device is always 0. Should be depending on where the main disk is
// to run where the user left off
void ext2_init(void)
{
    if (sizeof(ext2_super_t) != 1024) {
        abort("ext2_super_t should ALWAYS be 1024");
    }

    block_device_t* d = get_device(0);
    if (ext2_mount(d) < 0) {
        abort("Error occured in ext2_mount");
    }
}
