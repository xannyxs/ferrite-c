#ifndef FS_EXT2_H
#define FS_EXT2_H

#include "drivers/block/device.h"
#include "types.h"

#define EXT2_MAGIC 0xEF53
#define MAX_EXT2_MOUNTS 8

#define FIFO 0x1000
#define CHARACTER_DEVICE 0x2000
#define DIRECTORY 0x4000
#define BLOCK_DEVICE 0x6000
#define REGULAR_FILE 0x8000
#define SYMBOLIC_LINK 0xA000
#define UNIX_SOCKET 0xC000

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

typedef struct ext2_block_group_descriptor {
    u32 bg_block_bitmap;
    u32 bg_inode_bitmap;
    u32 bg_inode_table;
    u16 bg_free_blocks_count;
    u16 bg_free_inodes_count;
    u16 bg_used_dirs_count;
    u16 bg_pad;

    u8 __reserved[12];
} __attribute__((packed)) ext2_block_group_descriptor_t;

typedef struct ext2_inode {
    u16 i_mode;
    u16 i_uid;
    u32 i_size;
    u32 i_atime;
    u32 i_ctime;
    u32 i_mtime;
    u32 i_dtime;
    u16 i_gid;
    u16 i_links_count;
    u32 i_blocks;
    u32 i_flags;
    u32 i_osd1;
    u32 i_block[15];
    u32 i_generation;
    u32 i_file_acl;
    u32 i_dir_acl;
    u32 i_faddr;
    u8 i_osd2[12];

    u8 __extended[128];
} __attribute__((packed)) ext2_inode_t;

typedef struct {
    u32 inode;
    u16 rec_len;
    u8 name_len;
    u8 file_type;
    u8 name[];
} __attribute__((packed)) ext2_entry_t;

typedef struct {
    block_device_t* m_device;

    ext2_super_t m_superblock;
    ext2_inode_t m_inode;

    u32 num_block_groups;
    ext2_block_group_descriptor_t* m_block_group;
} ext2_mount_t;

/* Super Block Functions */

s32 ext2_read_superblock(block_device_t* d, ext2_super_t* super);

/* inode Functions */

s32 ext2_read_inode(u32 index, ext2_inode_t* inode, ext2_mount_t* m);

/* Block Group Descriptor Table Functions */

s32 ext2_read_block_descriptor_table(block_device_t* d,
    ext2_block_group_descriptor_t* bgd, u32 num_blocks_groups);

/* EXT2 */

ext2_entry_t* ext2_read_directory(char* entry_name, ext2_mount_t* m);

void ext2_init(void);

#endif /* FS_EXT2_H */
