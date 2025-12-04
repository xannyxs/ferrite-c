# VFS Layer

The VFS (Virtual Filesystem) ensures that the kernel can interact with filesystems
on a high-level, regardless of the underlying implementation.

Like the `block_device` structure, the VFS uses polymorphism through function pointers.
This allows the VFS to perform common tasks like path parsing and error checking
before delegating operations to the underlying filesystem.

## Core Concepts

### What is an Inode?

An inode (index node) is the fundamental data structure representing a filesystem object,
whether a file, directory, device, or other entity. Each inode contains metadata
about the object and provides a way to access its contents.

Think of inodes as the kernel's way of tracking "things" on disk.
When you open `/home/user/file.txt`, the kernel traverses a chain of inodes:
root directory → `home` directory → `user` directory → `file.txt` file.

### Reference Counting

The `i_count` field tracks how many references exist to an inode. This is critical for memory management:

- Opening a file increments the count
- Closing a file decrements the count
- A process's working directory holds a reference
- When count reaches zero, the inode can be freed

This prevents freeing an inode that's still in use,
which would cause kernel panics.

## The vfs_inode Structure

```c
typedef struct vfs_inode {
    dev_t i_dev;                    // Device number
    unsigned long i_ino;            // Inode number
    uid_t i_uid;                    // Owner user ID
    gid_t i_gid;                    // Owner group ID
    u32 i_size;                     // Size in bytes
    u16 i_mode;                     // Type and permissions
    u16 i_count;                    // Reference count
    u16 i_links_count;              // Hard link count
    time_t i_atime;                 // Last access time
    time_t i_mtime;                 // Last modification time
    time_t i_ctime;                 // Last status change time
    
    vfs_superblock_t* i_sb;         // Pointer to superblock
    struct inode_operations* i_op;   // Operations for this inode
    
    union {
        struct ext2_inode* i_ext2;   // Filesystem-specific data
    } u;
} vfs_inode_t;
```

### Inode Operations

The `inode_operations` structure contains function pointers for operations
specific to this inode type:

```c
struct inode_operations {
    int (*lookup)(struct vfs_inode*, const char*, int, struct vfs_inode**);
    int (*create)(struct vfs_inode*, const char*, int, mode_t);
    int (*mkdir)(struct vfs_inode*, const char*, int, mode_t);
    int (*rmdir)(struct vfs_inode*, const char*, int);
    int (*unlink)(struct vfs_inode*, const char*, int);
    // ... more operations
};
```

Different inode types support different operations:

- **Directories**: `lookup`, `mkdir`, `rmdir`
- **Regular files**: `read`, `write`, `truncate`

The VFS layer checks that operations are valid for the inode type before calling them.

## The vfs_superblock Structure

```c
typedef struct {
    dev_t s_dev;                       // Device this superblock describes
    unsigned long s_blocksize;         // Block size (e.g., 1024, 4096)
    struct vfs_inode* s_root_node;     // Root directory inode
    struct super_operations* s_op;     // Superblock operations
    
    union {
        struct {
            struct ext2_superblock* s_es;
            struct ext2_block_group_descriptor* s_group_desc;
            u32 s_groups_count;
        } ext2_sb;
    } u;
} vfs_superblock_t;
```

The superblock represents a mounted filesystem.
Each block device has one superblock, which serves as the entry point for that filesystem.

### Key Responsibilities

**Device Association:**

- Links the VFS layer to a specific block device via `s_dev`
- All inodes from this filesystem point back to this superblock

**Root Entry Point:**

- `s_root_node` is the starting point for path resolution
- For the root filesystem, this is `/`
- For mounted filesystems, this is the mount point

**Block Size:**

- `s_blocksize` defines the fundamental allocation unit
- Common values: 1024, 2048, 4096 bytes
- Must match the underlying filesystem's block size

**Filesystem-Specific Data:**

- The union contains structures specific to the filesystem type
- For ext2: superblock, block group descriptors, and group count
- These are read from disk during mount

## Per-Process Directory Context

Each process maintains two directory inodes:

```c
struct process {
    vfs_inode_t* root;  // Root directory (usually /)
    vfs_inode_t* pwd;   // Current working directory
};
```

**`root`:** The process's root directory, normally `/`.

**`pwd`:** The current working directory, changed with `chdir()`. Relative paths are resolved from here.

Both hold references to their inodes, which are released when the
process exits or changes directories.

## Design Philosophy

The VFS layer provides several key benefits. It offers abstraction by allowing
user programs and kernel code to work with filesystems without knowing
their implementation details. The same `open()` syscall works identically
regardless of whether it's ext2, FAT32, or any other filesystem.

This design also provides flexibility, as new filesystems can be added by simply
implementing the required operation structures without modifying any existing calling code.
The VFS promotes isolation by handling common tasks like path parsing
and permission checks in one place, reducing code duplication across different
filesystem implementations. Finally, it enhances safety through reference counting,
which prevents use-after-free bugs by ensuring inodes aren't freed while
they're still in use by processes or the system.
