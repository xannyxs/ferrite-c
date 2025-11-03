#include "fs/ext2/ext2.h"
#include "arch/x86/time/time.h"
#include "drivers/block/device.h"
#include "drivers/printk.h"
#include "fs/vfs.h"
#include "lib/math.h"
#include "lib/stdlib.h"
#include "lib/string.h"
#include "memory/kmalloc.h"
#include "sys/file/inode.h"
#include "sys/file/stat.h"
#include "types.h"

// TODO:
// - ext2_rename (for dirs)
// - ext2_directory_rename - create directory if it does not exist

ext2_mount_t ext2_mounts[MAX_EXT2_MOUNTS];

static vfs_inode_t* ext2_lookup(vfs_inode_t* inode, char const* name);
static vfs_inode_t* ext2_inode_get(u32 inode_num);
static void ext2_inode_put(vfs_inode_t* inode);
static int ext2_read(vfs_inode_t*, void* buff, u32 offset, u32 len);
static int ext2_write(vfs_inode_t* vfs, void const* buff, u32 offset, u32 len);

static struct inode_operations ext2_fops = {
    .lookup = ext2_lookup,
    .inode_get = ext2_inode_get,
    .inode_put = ext2_inode_put,
    .read = ext2_read,
    .write = ext2_write,
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

s32 ext2_write_directory(vfs_inode_t* vfs)
{
    ext2_mount_t* m = &ext2_mounts[0];
    block_device_t* d = m->m_device;
    if (!d) {
        printk("%s: device is NULL\n", __func__);
        return -1;
    }

    if (!d->d_op || !d->d_op->write) {
        printk("%s: device has no write operation\n", __func__);
        return -1;
    }

    s32 node_num = find_free_inode(m);
    if (node_num < 0) {
        return -1;
    }
    vfs->i_ino = node_num;

    s32 block_num = find_free_block(m);
    if (block_num < 0) {
        return -1;
    }

    ext2_inode_t* node = kmalloc(sizeof(ext2_inode_t));
    if (!node) {
        return -1;
    }

    node->i_ctime = getepoch();

    ext2_entry_t* entry = kmalloc(sizeof(ext2_entry_t));
    if (!entry) {
        kfree(node);
        return -1;
    }

    return 0;
}

int ext2_write(vfs_inode_t* vfs, void const* buff, u32 offset, u32 len)
{
    if (!vfs || !vfs->i_fs_specific) {
        return -1;
    }

    ext2_inode_t* node = vfs->i_fs_specific;
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

vfs_inode_t* ext2_create(vfs_inode_t* parent, char const* name, u32 mode)
{
    if (!parent) {
        return NULL;
    }

    ext2_mount_t* m = &ext2_mounts[0];

    s32 node_num = find_free_inode(m);
    if (node_num < 0) {
        return NULL;
    }

    if (mark_inode_allocated(m, node_num) < 0) {
        return NULL;
    }

    vfs_inode_t* new = kmalloc(sizeof(vfs_inode_t));
    if (!new) {
        return NULL;
    }

    ext2_inode_t* node = kmalloc(sizeof(ext2_inode_t));
    if (!node) {
        kfree(new);
        return NULL;
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
    new->i_fs_specific = node;

    ext2_entry_t entry = { 0 };
    entry.inode = node_num;
    entry.name_len = strlen(name);
    entry.file_type = mode & S_IFMT;
    memcpy(entry.name, name, entry.name_len);

    entry.rec_len = sizeof(ext2_entry_t) + entry.name_len;
    if (ext2_entry_write(m, &entry, parent->i_ino) < 0) {
        kfree(node);
        kfree(new);
        return NULL;
    }

    if (S_ISDIR(mode) == true) {
        node->i_links_count = 2;

        char const* current_str = ".";
        ext2_entry_t current_entry = { 0 };
        current_entry.inode = node_num;
        current_entry.name_len = strlen(current_str);
        current_entry.file_type = mode & S_IFMT;
        memcpy(current_entry.name, current_str, current_entry.name_len);

        current_entry.rec_len = sizeof(ext2_entry_t) + current_entry.name_len;
        if (ext2_entry_write(m, &current_entry, new->i_ino) < 0) {
            kfree(node);
            kfree(new);
            return NULL;
        }

        char const* parent_str = "..";
        ext2_entry_t parent_entry = { 0 };
        parent_entry.inode = parent->i_ino;
        parent_entry.name_len = strlen(parent_str);
        parent_entry.file_type = mode & S_IFMT;
        memcpy(parent_entry.name, parent_str, parent_entry.name_len);

        parent_entry.rec_len = sizeof(ext2_entry_t) + parent_entry.name_len;
        if (ext2_entry_write(m, &parent_entry, new->i_ino) < 0) {
            kfree(node);
            kfree(new);
            return NULL;
        }

        ext2_inode_t parent_inode = { 0 };
        if (ext2_read_inode(m, parent->i_ino, &parent_inode) < 0) {
            kfree(node);
            kfree(new);
            return NULL;
        }

        parent_inode.i_links_count += 1;
        parent_inode.i_ctime = now;
        parent_inode.i_mtime = now;

        if (ext2_write_inode(m, parent->i_ino, &parent_inode) < 0) {
            kfree(node);
            kfree(new);
            return NULL;
        }
    }

    if (ext2_write_inode(m, node_num, node) < 0) {
        kfree(node);
        kfree(new);
        return NULL;
    }

    return new;
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

    if (!vfs->i_fs_specific) {
        return -1;
    }
    ext2_inode_t* inode = (ext2_inode_t*)vfs->i_fs_specific;
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
    new->i_fs_specific = (void*)found_inode;
    new->i_op = &ext2_fops;

    return new;
}

void ext2_inode_put(vfs_inode_t* inode)
{
    if (inode && inode->i_fs_specific) {
        kfree(inode->i_fs_specific);
    }
}

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

    vfs_inode_t* new = kmalloc(sizeof(inode_t));
    if (!new) {
        kfree(entry);
        kfree(found_inode);
        return NULL;
    }

    new->i_ino = entry->inode;
    new->i_size = found_inode->i_size;
    new->i_mode = found_inode->i_mode;
    new->i_fs_specific = (void*)found_inode;
    new->i_op = &ext2_fops;

    kfree(entry);

    return new;
}

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

    vfs_inode_t node = { 0 };
    node.i_ino = 2;
    node.i_size = 1000;
    memcpy(node.i_name, "test.txt", 9);

    vfs_inode_t* result1 = ext2_lookup(&node, ".");
    if (result1) {
        printk(
            "Found '.': inode=%u, size=%u\n", result1->i_ino, result1->i_size
        );
    } else {
        printk("Failed to find '.'\n");
    }

    char* buff = "Hello this is a test";
    if (ext2_write(&node, buff, 0, strlen(buff)) < 0) {
        printk("Error in write\n");
    }

    vfs_inode_t* result2 = ext2_lookup(&node, "something");
    if (result2) {
        printk(
            "Found 'something': inode=%u, size=%u\n", result2->i_ino,
            result2->i_size
        );
    } else {
        printk("Failed to find 'something'\n");
    }

    vfs_inode_t* parent = ext2_create(result1, "test.txt", 0);
    if (!parent) {
        printk("Error in create\n");
        return;
    }

    if (ext2_write(parent, buff, 0, strlen(buff)) < 0) {
        printk("Error in write\n");
        return;
    }

    result2 = ext2_lookup(&node, "something");
    if (result2) {
        printk(
            "Found 'something': inode=%u, size=%u\n", result2->i_ino,
            result2->i_size
        );
    } else {
        printk("Failed to find 'something'\n");
    }
}
