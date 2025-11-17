#include "drivers/block/device.h"
#include "drivers/printk.h"
#include "fs/ext2/ext2.h"
#include "fs/vfs.h"
#include "lib/math.h"
#include "lib/string.h"
#include "memory/kmalloc.h"
#include <ferrite/types.h>

// TODO: If all blocks are full it should grow
s32 ext2_entry_write(ext2_mount_t* m, ext2_entry_t* entry, u32 parent_ino)
{
    block_device_t* d = m->m_device;
    if (!d) {
        printk("%s: device is NULL\n", __func__);
        return -1;
    }

    if (!d->d_op || !d->d_op->write) {
        printk("%s: device has no write operation\n", __func__);
        return -1;
    }

    ext2_inode_t node = { 0 };
    if (ext2_read_inode(m, parent_ino, &node) < 0) {
        return -1;
    }

    u32 const count = m->m_block_size / d->sector_size;
    u32 const max_block = CEIL_DIV(node.i_size, m->m_block_size);

    u32 offset = 0;
    ext2_entry_t* e = NULL;
    u8 buff[m->m_block_size];
    u32 sector_pos = 0;
    for (u32 i = 0; i < max_block; i += 1) {
        u32 const addr = node.i_block[i] * m->m_block_size;
        sector_pos = addr / d->sector_size;

        if (d->d_op->read(d, sector_pos, count, buff, m->m_block_size) < 0) {
            return -1;
        }

        while (offset < m->m_block_size
               && (i * m->m_block_size) + offset <= node.i_size) {
            e = (ext2_entry_t*)&buff[offset];

            u32 size_of_entry
                = ALIGN(sizeof(ext2_entry_t) + e->name_len, sizeof(e->inode));
            if (e->rec_len > size_of_entry) {
                break;
            }
            offset += e->rec_len;
        }

        if (offset < m->m_block_size) {
            break;
        }

        offset = 0;
        e = NULL;
    }

    if (!e) {
        return -1;
    }

    u32 const original_rec_len = e->rec_len;
    u32 const last_aligned_size
        = ALIGN(sizeof(ext2_entry_t) + e->name_len, sizeof(entry->inode));

    e->rec_len = last_aligned_size;
    entry->rec_len = original_rec_len - last_aligned_size;

    offset += e->rec_len;
    memcpy(&buff[offset], entry, sizeof(ext2_entry_t) + entry->name_len);

    if (d->d_op->write(d, sector_pos, count, buff, m->m_block_size) < 0) {
        return -1;
    }

    return 0;
}

s32 ext2_find_entry(
    vfs_inode_t* dir,
    char const* const name,
    int name_len,
    ext2_entry_t** result
)
{
    block_device_t* d = get_device(0); // FIXME
    if (!d) {
        printk("%s: device is NULL\n", __func__);
        return -1;
    }

    if (!d->d_op || !d->d_op->read) {
        printk("%s: device has no read operation\n", __func__);
        return -1;
    }

    ext2_inode_t* node = dir->u.i_ext2;
    vfs_superblock_t* sb = dir->i_sb;

    u32 const count = sb->s_blocksize / d->sector_size;

    u32 blocks = (dir->i_size + sb->s_blocksize - 1) / sb->s_blocksize;
    for (int i = 0; (u32)i < blocks && i < 12; i += 1) {
        if (!node->i_block[i]) {
            break;
        }

        u32 addr = node->i_block[i] * sb->s_blocksize;
        u32 sector_pos = addr / d->sector_size;

        u8 buff[sb->s_blocksize];
        if (d->d_op->read(d, sector_pos, count, buff, sb->s_blocksize) < 0) {
            return -1;
        }

        u32 offset = 0;
        while (offset < sb->s_blocksize) {
            ext2_entry_t* e = (ext2_entry_t*)&buff[offset];

            if (name_len == e->name_len && *name == (char)*e->name
                && strncmp(name, (char*)e->name, e->name_len) == 0) {

                *result = kmalloc(sizeof(ext2_entry_t) + e->name_len);
                memcpy(
                    *result, &buff[offset], sizeof(ext2_entry_t) + e->name_len
                );
                return 0;
            }
            offset += e->rec_len;
        }
    }

    return -1;
}
