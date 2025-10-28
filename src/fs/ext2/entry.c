#include "drivers/block/device.h"
#include "drivers/printk.h"
#include "fs/ext2/ext2.h"
#include "lib/string.h"
#include "memory/kmalloc.h"
#include "types.h"

s32 ext2_read_entry(
    ext2_mount_t* m,
    ext2_entry_t** entry,
    u32 inode_num,
    char const* entry_name
)
{
    block_device_t* d = m->m_device;
    if (!d) {
        printk("%s: device is NULL\n", __func__);
        return -1;
    }

    if (!d->d_op || !d->d_op->read) {
        printk("%s: device has no read operation\n", __func__);
        return -1;
    }

    ext2_inode_t dir_inode = { 0 };
    ext2_read_inode(m, inode_num, &dir_inode);

    u32 addr = dir_inode.i_block[0] * m->m_block_size;
    u32 sector_pos = addr / d->sector_size;

    u8 buff[1024];
    if (d->d_op->read(d, sector_pos, dir_inode.i_blocks, buff, dir_inode.i_size)
        < 0) {
        printk(
            "%s: failed to read from device (LBA %u, "
            "count %u)\n",
            __func__, sector_pos, 1
        );
        return -1;
    }

    u32 offset = 0;
    size_t path_len = strlen(entry_name);
    while (offset < dir_inode.i_size) {
        ext2_entry_t* e = (ext2_entry_t*)&buff[offset];

        if (path_len == e->name_len && *entry_name == (char)*e->name
            && strncmp(entry_name, (char*)e->name, e->name_len) == 0) {

            *entry = kmalloc(e->rec_len);
            memcpy(*entry, &buff[offset], e->rec_len);
            return 0;
        }
        offset += e->rec_len;
    }

    return -1;
}
