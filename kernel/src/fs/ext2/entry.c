#include "drivers/block/device.h"
#include "drivers/printk.h"
#include "fs/ext2/ext2.h"
#include "fs/vfs.h"
#include "lib/math.h"
#include "memory/kmalloc.h"

#include <ferrite/errno.h>
#include <ferrite/string.h>
#include <ferrite/types.h>

int ext2_delete_entry(vfs_inode_t* dir, ext2_entry_t* entry)
{
    block_device_t* d = get_device(dir->i_sb->s_dev);
    if (!d || !d->d_op || !d->d_op->read) {
        printk("%s: device has no read operation\n", __func__);
        return -1;
    }

    ext2_inode_t* node = dir->u.i_ext2;
    vfs_superblock_t* sb = dir->i_sb;

    u32 const count = sb->s_blocksize / d->d_sector_size;
    u32 blocks = (dir->i_size + sb->s_blocksize - 1) / sb->s_blocksize;

    u8* buff = kmalloc(sb->s_blocksize);
    if (!buff) {
        return -ENOMEM;
    }

    for (int i = 0; (u32)i < blocks && i < 12; i += 1) {
        if (!node->i_block[i]) {
            break;
        }
        u32 addr = node->i_block[i] * sb->s_blocksize;
        u32 sector_pos = addr / d->d_sector_size;

        if (d->d_op->read(d, sector_pos, count, buff, sb->s_blocksize) < 0) {
            kfree(buff);
            return -EIO;
        }

        u32 offset = 0;
        ext2_entry_t* prev = NULL;

        while (offset < sb->s_blocksize) {
            ext2_entry_t* e = (ext2_entry_t*)&buff[offset];

            if (e->rec_len == 0 || e->rec_len + offset > sb->s_blocksize) {
                kfree(buff);
                return -EUCLEAN;
            }

            if (entry->name_len == e->name_len
                && strncmp((char*)entry->name, (char*)e->name, e->name_len)
                    == 0) {

                if (prev) {
                    prev->rec_len += e->rec_len;
                } else {
                    e->inode = 0;
                }

                if (d->d_op->write(d, sector_pos, count, buff, sb->s_blocksize)
                    < 0) {
                    kfree(buff);
                    return -EIO;
                }
                kfree(buff);
                return 0;
            }

            prev = e;
            offset += e->rec_len;
        }
    }

    kfree(buff);
    return -ENOENT;
}

s32 ext2_write_entry(vfs_inode_t* dir, ext2_entry_t* entry)
{
    block_device_t* d = get_device(dir->i_dev);
    if (!d || !d->d_op || !d->d_op->write || !d->d_op->read) {
        return -EIO;
    }

    vfs_superblock_t* sb = dir->i_sb;
    ext2_inode_t* ext2_node = dir->u.i_ext2;
    u32 const count = sb->s_blocksize / d->d_sector_size;
    u32 const max_block = CEIL_DIV(dir->i_size, sb->s_blocksize);

    u8* buff = kmalloc(sb->s_blocksize);
    if (!buff) {
        return -ENOMEM;
    }

    u32 block_num = 0;
    u32 offset = 0;
    u32 sector_pos = 0;
    ext2_entry_t* e = NULL;

    for (block_num = 0; block_num < max_block && block_num < 12; block_num++) {
        if (!ext2_node->i_block[block_num]) {
            break;
        }

        u32 const addr = ext2_node->i_block[block_num] * sb->s_blocksize;
        sector_pos = addr / d->d_sector_size;

        if (d->d_op->read(d, sector_pos, count, buff, sb->s_blocksize) < 0) {
            kfree(buff);
            return -EIO;
        }

        offset = 0;
        while (offset < sb->s_blocksize
               && offset < dir->i_size - (block_num * sb->s_blocksize)) {
            e = (ext2_entry_t*)&buff[offset];

            if (e->rec_len == 0 || offset + e->rec_len > sb->s_blocksize) {
                kfree(buff);
                return -EUCLEAN;
            }

            u32 actual_size = ALIGN(sizeof(ext2_entry_t) + e->name_len, 4);

            if (e->inode == 0 || e->rec_len > actual_size) {
                goto found_space;
            }

            offset += e->rec_len;
        }
    }

    kfree(buff);
    return -ENOSPC;

found_space:
    if (e->inode == 0) {
        memcpy(&buff[offset], entry, sizeof(ext2_entry_t) + entry->name_len);
    } else {
        u32 const original_rec_len = e->rec_len;
        u32 const actual_size = ALIGN(sizeof(ext2_entry_t) + e->name_len, 4);

        e->rec_len = actual_size;
        offset += actual_size;

        entry->rec_len = original_rec_len - actual_size;
        memcpy(&buff[offset], entry, sizeof(ext2_entry_t) + entry->name_len);
    }

    int retval = d->d_op->write(d, sector_pos, count, buff, sb->s_blocksize);
    kfree(buff);

    if (retval < 0) {
        return -EIO;
    }

    u32 new_size = (block_num * sb->s_blocksize) + offset + entry->rec_len;
    if (new_size > dir->i_size) {
        dir->i_size = new_size;
        ext2_node->i_size = new_size;

        if (sb->s_op->write_inode(dir) < 0) {
            return -EIO;
        }
    }

    return 0;
}

s32 ext2_find_entry_by_ino(
    vfs_inode_t* dir,
    unsigned long ino,
    ext2_entry_t** result
)
{
    block_device_t* d = get_device(dir->i_sb->s_dev);
    if (!d || !d->d_op || !d->d_op->read) {
        printk("%s: device has no read operation\n", __func__);
        return -1;
    }

    ext2_inode_t* node = dir->u.i_ext2;
    vfs_superblock_t* sb = dir->i_sb;

    u32 const count = sb->s_blocksize / d->d_sector_size;
    u32 blocks = (dir->i_size + sb->s_blocksize - 1) / sb->s_blocksize;

    u8* buff = kmalloc(sb->s_blocksize);
    if (!buff) {
        return -ENOMEM;
    }

    for (int i = 0; (u32)i < blocks && i < 12; i += 1) {
        if (!node->i_block[i]) {
            break;
        }

        u32 addr = node->i_block[i] * sb->s_blocksize;
        u32 sector_pos = addr / d->d_sector_size;

        if (d->d_op->read(d, sector_pos, count, buff, sb->s_blocksize) < 0) {
            kfree(buff);
            return -1;
        }

        u32 offset = 0;
        while (offset < sb->s_blocksize) {
            ext2_entry_t* e = (ext2_entry_t*)&buff[offset];

            if (e->inode == ino) {
                *result = kmalloc(sizeof(ext2_entry_t) + e->name_len);
                memcpy(
                    *result, &buff[offset], sizeof(ext2_entry_t) + e->name_len
                );
                kfree(buff);
                return 0;
            }
            offset += e->rec_len;
        }
    }

    kfree(buff);
    return -1;
}

s32 ext2_find_entry(
    vfs_inode_t* dir,
    char const* const name,
    int name_len,
    ext2_entry_t** result
)
{
    block_device_t* d = get_device(dir->i_sb->s_dev);
    if (!d || !d->d_op || !d->d_op->read) {
        printk("%s: device has no read operation\n", __func__);
        return -1;
    }

    ext2_inode_t* node = dir->u.i_ext2;
    vfs_superblock_t* sb = dir->i_sb;

    u32 const count = sb->s_blocksize / d->d_sector_size;
    u32 blocks = (dir->i_size + sb->s_blocksize - 1) / sb->s_blocksize;

    u8* buff = kmalloc(sb->s_blocksize);
    if (!buff) {
        return -ENOMEM;
    }

    for (int i = 0; (u32)i < blocks && i < 12; i += 1) {
        if (!node->i_block[i]) {
            break;
        }
        u32 addr = node->i_block[i] * sb->s_blocksize;
        u32 sector_pos = addr / d->d_sector_size;

        if (d->d_op->read(d, sector_pos, count, buff, sb->s_blocksize) < 0) {
            kfree(buff);
            return -EIO;
        }

        u32 offset = 0;
        while (offset < sb->s_blocksize) {
            ext2_entry_t* e = (ext2_entry_t*)&buff[offset];

            if (e->rec_len == 0 || offset + e->rec_len > sb->s_blocksize) {
                kfree(buff);
                return -EUCLEAN;
            }

            if (e->inode != 0 && name_len == e->name_len
                && strncmp(name, (char*)e->name, e->name_len) == 0) {
                *result = kmalloc(sizeof(ext2_entry_t) + e->name_len);
                if (!*result) {
                    kfree(buff);
                    return -ENOMEM;
                }
                memcpy(*result, e, sizeof(ext2_entry_t) + e->name_len);
                kfree(buff);
                return 0;
            }

            offset += e->rec_len;
        }
    }

    kfree(buff);
    return -ENOENT;
}
