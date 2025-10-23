#include "fs/ext2/ext2.h"
#include "drivers/block/device.h"
#include "drivers/printk.h"
#include "lib/math.h"
#include "lib/stdlib.h"
#include "memory/kmalloc.h"
#include "types.h"

ext2_mount_t ext2_mounts[MAX_EXT2_MOUNTS];

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
    ext2_read_superblock(d, &m->m_superblock);

    m->num_block_groups = CEIL_DIV(
        m->m_superblock.s_blocks_count, m->m_superblock.s_blocks_per_group);
    u32 bgd_bytes = m->num_block_groups * sizeof(ext2_block_group_descriptor_t);
    u32 bgd_sectors = CEIL_DIV(bgd_bytes, d->sector_size);

    m->m_block_group = kmalloc(bgd_sectors * d->sector_size);
    if (!m->m_block_group) {
        return -1;
    }

    ext2_read_block_descriptor_table(d, m->m_block_group, m->num_block_groups);

    ext2_read_inode(2, m, d);

    return 0;
}

void ext2_umount(void) { }

// TODO:
// Should I already mount
// get_device is always 0. Should be depending on where the main disk is to run
// where the user left off
void ext2_init(void)
{
    if (sizeof(ext2_super_t) != 1024) {
        abort("ext2_super_t should ALWAYS be 1024");
    }

    block_device_t* d = get_device(0);
    ext2_mount(d);
}
