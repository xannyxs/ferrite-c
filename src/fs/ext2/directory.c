#include "drivers/block/device.h"
#include "drivers/printk.h"
#include "fs/ext2/ext2.h"
#include "lib/string.h"
#include "memory/kmalloc.h"
#include "types.h"

s32 ext2_entry_find(char* path, ext2_mount_t* m)
{
    (void)path;
    (void)m;
    return 0;
}

ext2_entry_t* ext2_read_directory(char* entry_name, ext2_mount_t* m)
{
    block_device_t* d = m->m_device;
    if (!d) {
        printk("%s: device is NULL\n", __func__);
        return NULL;
    }

    if (!d->d_op || !d->d_op->read) {
        printk("%s: device has no read operation\n", __func__);
        return NULL;
    }

    ext2_inode_t inode = m->m_inode;
    ext2_super_t s = m->m_superblock;

    u32 addr = inode.i_block[0] * (1024 << s.s_log_block_size);
    u32 sector_pos = addr / d->sector_size;

    u8* buff = kmalloc(1024);
    if (d->d_op->read(d, sector_pos, inode.i_blocks, buff, inode.i_size) < 0) {
        printk("%s: failed to read from device (LBA %u, "
               "count %u)\n",
            __func__, sector_pos, 1);
        return NULL;
    }

    u32 offset = 0;
    while (offset < inode.i_size) {
        ext2_entry_t* entry = (ext2_entry_t*)&buff[offset];
        offset += entry->rec_len;

        size_t path_len = strlen(entry_name);
        if (path_len == entry->name_len && *entry_name == (char)*entry->name
            && strncmp(entry_name, (char*)entry->name, entry->name_len) == 0) {

            return entry;
        }
    }

    return NULL;
}
