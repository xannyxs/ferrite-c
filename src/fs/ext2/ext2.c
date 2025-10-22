#include "fs/ext2/ext2.h"
#include "drivers/block/device.h"
#include "drivers/printk.h"
#include "lib/stdlib.h"
#include "types.h"

ext2_mount_t ext2_mounts[MAX_EXT2_MOUNTS];

s32 ext2_mount(block_device_t* d)
{
    if (!d) {
        return -1;
    }

    ext2_mount_t* m = NULL;

    for (int i = 0; i < MAX_EXT2_MOUNTS; i += 1) {
        if (!ext2_mounts[i].s_device) {
            m = &ext2_mounts[i];
            break;
        }
    }

    if (!m) {
        return -1;
    }

    m->s_device = d;
    ext2_read_superblock(d, &m->s_superblock);
    // m->s_inode = NULL; // WIP

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
