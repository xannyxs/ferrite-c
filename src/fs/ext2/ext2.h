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

/* EXT2 specific flags */
#define EXT2_SECRM_FL 0x00000001    // secure deletion
#define XT2_UNRM_FL 0x00000002      // record for undelete
#define XT2_COMPR_FL 0x00000004     // compressed file
#define XT2_SYNC_FL 0x00000008      // synchronous updates
#define XT2_IMMUTABLE_FL 0x00000010 // immutable file
#define XT2_APPEND_FL 0x00000020    // append only
#define XT2_NODUMP_FL 0x00000040    // do not dump / delete file
#define XT2_NOATIME_FL 0x00000080   // do not update.i_atime
/* Reserved for compression usage */
#define EXT2_DIRTY_FL 0x00000100    // Dirty(modified)
#define EXT2_COMPRBLK_FL 0x00000200 // compressed blocks
#define XT2_NOCOMPR_FL 0x00000400   // access raw compressed data
#define XT2_ECOMPR_FL 0x00000800    // compression error
/* End of compression flags */
#define XT2_BTREE_FL 0x00001000    // b - tree format directory
#define XT2_INDEX_FL 0x00001000    // hash indexed directory
#define XT2_IMAGIC_FL 0x00002000   // AFS directory
#define XT2_RESERVED_FL 0x80000000 // reserved for ext2 library

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
    u32 i_atime; // Access time (updated on read)
    u32 i_ctime; // Change time (updated on metadata/inode changes)
    u32 i_mtime; // Modification time (updated on content changes)
    u32 i_dtime; // Deletion time (set when inode is freed/deleted)
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

    u32 m_block_size;
    u32 num_block_groups;
    ext2_block_group_descriptor_t* m_block_group;
} ext2_mount_t;

/* Super Block Functions */
s32 ext2_superblock_read(ext2_mount_t* m);

s32 ext2_superblock_write(ext2_mount_t* m);

/* Block Group Descriptor Table Functions */
s32 ext2_block_group_descriptors_read_all(
    ext2_mount_t* m,
    ext2_block_group_descriptor_t* bgd,
    u32 num_block_groups
);

s32 ext2_block_group_descriptors_write(ext2_mount_t* m, u32 bgd_index);

/* Block Functions */
int mark_block_allocated(ext2_mount_t* m, u32 block_num);

int mark_block_free(ext2_mount_t* m, u32 block_num);

s32 ext2_write_block(
    ext2_mount_t* m,
    u32 block_num,
    void const* buff,
    u32 offset,
    u32 len
);

/* Inode Functions */
s32 ext2_read_inode(ext2_mount_t* m, u32 inode_num, ext2_inode_t* inode);

s32 ext2_write_inode(ext2_mount_t* m, u32 inode_num, ext2_inode_t* inode);

int find_free_inode(ext2_mount_t* m);

int mark_inode_allocated(ext2_mount_t* m, u32 inode_num);

int mark_inode_free(ext2_mount_t* m, u32 inode_num);

/* Directory Functions */
s32 ext2_read_entry(
    ext2_mount_t* m,
    ext2_entry_t** entry,
    u32 inode_num,
    char const* entry_name
);

s32 ext2_entry_write(ext2_mount_t* m, ext2_entry_t* entry, u32 parent_ino);

/* General Functio */
int find_free_bit_in_bitmap(u8 const* bitmap, u32 size);

int read_block(ext2_mount_t* m, u8* buff, u32 block_num);

int find_free_block(ext2_mount_t* m);

void ext2_init(void);

#endif /* FS_EXT2_H */
