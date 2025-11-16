#include "fs/ext2/ext2.h"
#include "drivers/block/device.h"
#include "fs/vfs.h"
#include "lib/math.h"
#include "lib/stdlib.h"
#include "memory/kmalloc.h"

#include <ferrite/types.h>
#include <lib/string.h>

ext2_mount_t ext2_mounts[MAX_EXT2_MOUNTS];

static struct vfs_inode* ext2_inode_get(u32 inode_num);

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

struct vfs_inode* ext2_inode_get(u32 inode_num)
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

    struct vfs_inode* new = kmalloc(sizeof(struct vfs_inode));
    if (!new) {
        kfree(found_inode);
        return NULL;
    }

    new->i_ino = inode_num;
    new->i_size = found_inode->i_size;
    new->i_mode = found_inode->i_mode;
    new->u.i_ext2 = found_inode;
    // new->i_op = &ext2_fops;

    return new;
}

struct vfs_inode* ext2_get_root_node(void) { return ext2_inode_get(2); }

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
