#ifndef FS_EXT2_H
#define FS_EXT2_H

#include "drivers/block/device.h"
#include "types.h"

#define EXT2_MAGIC 0xEF53
#define MAX_EXT2_MOUNTS 8

typedef struct {
    u32 s_inodes_count;
    u32 s_blocks_count;
    u32 s_r_blocks_count; // Reserved blocks for superuser
    u32 s_free_blocks_count;
    u32 s_free_inodes_count;
    u32 s_first_data_block; // NOT always zero
    u32 s_log_block_size;   // Block size = 1024 << s_log_block_size
    u32 s_log_frag_size;    // Fragment size = 1024 << s_log_frag_size
    u32 s_blocks_per_group;
    u32 s_frags_per_group;
    u32 s_inodes_per_group;
    u32 s_mtime; // Last mount time (POSIX)
    u32 s_wtime; // Last write time (POSIX)
    u16 s_mnt_count;
    u16 s_max_mnt_count;
    u16 s_magic; // Magic signature (0xEF53)
    u16 s_state;
    u16 s_errors;
    u16 s_minor_rev_level;
    u32 s_lastcheck;     // Last check time(POSIX)
    u32 s_checkinterval; // Time between checks
    u32 s_creator_os;
    u32 s_rev_level;
    u16 s_def_resuid;
    u16 s_def_resgid;

    // - EXT2_DYNAMIC_REV Specific --
    u32 s_first_ino;
    u16 s_inode_size;
    u16 s_block_group_nr;
    u32 s_feature_compat;
    u32 s_feature_incompat;
    u32 s_feature_ro_compat;

    u8 s_uuid[16];
    u8 s_volume_name[16];
    u8 s_last_mounted[64];
    u32 s_algo_bitmap;

    // -- Performance Hints --
    u8 s_prealloc_blocks;
    u8 s_prealloc_dir_blocks;
    u16 __padding1;

    // -- Journaling Support (ext3) --
    u8 s_journal_uuid[16];
    u32 s_journal_inum;
    u32 s_journal_dev;
    u32 s_last_orphan;

    // -- Directory Indexing Support --
    u32 s_hash_seed[4];
    u8 s_def_hash_version;
    u8 __padding2[3];

    // -- Other options --
    u32 s_default_mount_options;
    u32 s_first_meta_bg;
    u8 __reserved[760];
} __attribute__((packed)) ext2_super_t;

typedef struct ext2_inode {
    size_t size;
} ext2_inode_t;

typedef struct {
    block_device_t* s_device;
    ext2_super_t s_superblock;
    ext2_inode_t s_inode;
} ext2_mount_t;

/* Super Block Functions */

s32 ext2_read_superblock(block_device_t* d, ext2_super_t* super);

/* EXT2 */

void ext2_init(void);

#endif /* FS_EXT2_H */
