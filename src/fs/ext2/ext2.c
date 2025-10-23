#include "fs/ext2/ext2.h"
#include "drivers/block/device.h"
#include "drivers/printk.h"
#include "lib/math.h"
#include "lib/stdlib.h"
#include "lib/string.h"
#include "memory/kmalloc.h"
#include "sys/file/inode.h"
#include "types.h"

ext2_mount_t ext2_mounts[MAX_EXT2_MOUNTS];

struct inode_t {
    u32 i_ino;
    u32 i_mode;
    u32 size;
    void* fs_specific;
};

struct inode_t* ext2_lookup(struct inode_t* parent_dir, char const* name)
{
    ext2_mount_t* m = &ext2_mounts[0];

    ext2_inode_t parent_inode = { 0 };
    ext2_read_inode(parent_dir->i_ino, &parent_inode, m);

    ext2_entry_t* entry = ext2_read_directory(name, parent_inode, m);
    if (!entry) {
        return NULL;
    }

    ext2_inode_t* found_inode = kmalloc(sizeof(ext2_inode_t));
    if (!found_inode) {
        return NULL;
    }
    ext2_read_inode(entry->inode, found_inode, m);

    struct inode_t* new = kmalloc(sizeof(inode_t));
    if (!new) {
        kfree(found_inode);
        return NULL;
    }

    new->i_ino = entry->inode;
    new->size = found_inode->i_size;
    new->i_mode = found_inode->i_mode;
    new->fs_specific = (void*)found_inode;

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
    if (ext2_read_superblock(d, &m->m_superblock) < 0) {
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

    if (ext2_read_block_descriptor_table(
            d, m->m_block_group, m->num_block_groups)
        < 0) {
        return -1;
    }

    if (ext2_read_inode(2, &m->m_inode, m) < 0) {
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

    struct inode_t inode = { 0 };
    inode.i_mode = 1;
    inode.i_ino = 2;
    inode.size = 1024;
    inode.fs_specific = NULL;

    struct inode_t* result1 = ext2_lookup(&inode, ".");
    if (result1) {
        printk("Found '.': inode=%u, size=%u\n", result1->i_ino, result1->size);
    } else {
        printk("Failed to find '.'\n");
    }

    struct inode_t* result2 = ext2_lookup(&inode, "lost+found");
    if (result2) {
        printk("Found 'lost+found': inode=%u, size=%u\n", result2->i_ino,
            result2->size);
    } else {
        printk("Failed to find 'lost+found'\n");
    }
}
