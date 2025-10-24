#include "fs/ext2/ext2.h"
#include "drivers/block/device.h"
#include "drivers/printk.h"
#include "fs/vfs.h"
#include "lib/math.h"
#include "lib/stdlib.h"
#include "lib/string.h"
#include "memory/kmalloc.h"
#include "sys/file/inode.h"
#include "types.h"

ext2_mount_t ext2_mounts[MAX_EXT2_MOUNTS];

static vfs_inode_t* ext2_lookup(vfs_inode_t* inode, char const* name);
static vfs_inode_t* ext2_inode_get(u32 inode_num);
static void ext2_inode_put(vfs_inode_t* inode);

static struct inode_operations ext2_fops = {
    .lookup = ext2_lookup,
    .inode_get = ext2_inode_get,
    .inode_put = ext2_inode_put,
};

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
    new->size = found_inode->i_size;
    new->i_mode = found_inode->i_mode;
    new->fs_specific = (void*)found_inode;
    new->i_op = &ext2_fops;

    return new;
}

void ext2_inode_put(vfs_inode_t* inode)
{
    if (inode) {
        kfree(inode);
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
    new->size = found_inode->i_size;
    new->i_mode = found_inode->i_mode;
    new->fs_specific = (void*)found_inode;
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
    if (ext2_read_superblock(m, &m->m_superblock) < 0) {
        return -1;
    }

    m->num_block_groups = CEIL_DIV(
        m->m_superblock.s_blocks_count, m->m_superblock.s_blocks_per_group);
    u32 bgd_bytes = m->num_block_groups * sizeof(ext2_block_group_descriptor_t);
    u32 bgd_sectors = CEIL_DIV(bgd_bytes, d->sector_size);

    m->m_block_group = kmalloc(bgd_sectors * d->sector_size);
    if (!m->m_block_group) {
        return -1;
    }

    if (ext2_read_bgd_table(m, m->m_block_group, m->num_block_groups) < 0) {
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

    vfs_inode_t* result1 = ext2_lookup(2, ".");
    if (result1) {
        printk("Found '.': inode=%u, size=%u\n", result1->i_ino, result1->size);
    } else {
        printk("Failed to find '.'\n");
    }

    vfs_inode_t* result2 = ext2_lookup(result1->i_ino, "lost+found");
    if (result2) {
        printk("Found 'lost+found': inode=%u, size=%u\n", result2->i_ino,
            result2->size);
    } else {
        printk("Failed to find 'lost+found'\n");
    }
}
