#ifndef FS_EXT2_H
#define FS_EXT2_H

#include "fs/vfs.h"
#include <types.h>

struct vfs_inode;
struct vfs_dentry;

#define EXT2_MAGIC 0xEF53
#define EXT2_ROOT_INO 2

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

typedef struct ext2_superblock {
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

/* super.c */

vfs_superblock_t* ext2_superblock_read(vfs_superblock_t*, void*, int);

s32 ext2_superblock_write(vfs_superblock_t*);

s32 ext2_bgd_write(vfs_superblock_t* sb, u32 bgd_index);

/* balloc.c */

int ext2_new_block(vfs_inode_t const* node, int* err);

int ext2_free_block(vfs_inode_t* node, u32 block_num);

/* block.c */

s32 ext2_read_block(vfs_inode_t const*, u8*, u32);

s32 ext2_write_block(vfs_inode_t*, u32, void const*, u32, u32);

/* ialloc.c */

vfs_inode_t* ext2_new_inode(vfs_inode_t const* dir, int mode, int* err);

int ext2_free_inode(vfs_inode_t* dir);

/* entry.c */

int ext2_find_entry(
    struct vfs_inode* dir,
    char const* const name,
    int name_len,
    ext2_entry_t** result
);

s32 ext2_find_entry_by_ino(
    vfs_inode_t* dir,
    unsigned long ino,
    ext2_entry_t** result
);

s32 ext2_write_entry(vfs_inode_t* dir, ext2_entry_t* entry);

s32 ext2_delete_entry(vfs_inode_t* dir, ext2_entry_t* entry);

/* namei.c */

int ext2_create(
    vfs_inode_t* dir,
    char const* name,
    int len,
    int mode,
    vfs_inode_t** result
);

s32 ext2_lookup(struct vfs_inode*, char const*, int, struct vfs_inode**);

/* dir.c */
extern struct inode_operations ext2_dir_inode_operations;

/* file.c */
extern struct inode_operations ext2_file_inode_operations;

/* acl.c */
int ext2_permission(vfs_inode_t* node, int mask);

/* truncate.c */
extern int ext2_truncate(vfs_inode_t*, off_t);

static inline int find_free_bit_in_bitmap(u8 const* bitmap, u32 size)
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

#endif /* FS_EXT2_H */
