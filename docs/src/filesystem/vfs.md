# VFS Layer

The VFS (Virtual Filesystem) ensure that the kernel can easily
interact with the filesystem without caring which type of filesystem
it runs on the low-level.

It is a high-level implementation to let the Kernel understand the
filesystem. Just like the `block_device` struct it uses polymorphism
to not worry about the underlying filesystem. It just gives some basic
preperation like parsing or error checking before passing the node
to the underlying filesystem.

## The VFS Structure

There are two important structures that the VFS makes use of.
The first one is the `vfs_inode` struct. Each inode represents an entry.

It has information stored like size, modification time & most importantly,
which underlying node it is pointing to.

The other struct is the `vfs_superblock` struct. Each device has its
own unique superblock struct. This has some basic information like
the size of each block in the inode or which `block_device` struct
it points to. All inode points to atleast one superblock, but there
should only be one superblock per harddrive.

```c
typedef struct {
    dev_t s_dev;
    unsigned long s_blocksize;

    struct vfs_inode* s_root_node;

    struct super_operations* s_op;
    union {
        struct {
            struct ext2_superblock* s_es;
            struct ext2_block_group_descriptor* s_group_desc;
            u32 s_groups_count;
        } ext2_sb;
    } u;
} vfs_superblock_t;

typedef struct vfs_inode {
    dev_t i_dev;
    unsigned long i_ino;

    uid_t i_uid;
    gid_t i_gid;

    u32 i_size;
    u16 i_mode;
    u16 i_count;
    u16 i_links_count;

    time_t i_atime;
    time_t i_mtime;
    time_t i_ctime;

    vfs_superblock_t* i_sb;

    struct inode_operations* i_op;
    union {
        struct ext2_inode* i_ext2;
    } u;
} vfs_inode_t;
```
