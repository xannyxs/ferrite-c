# Kernel Filesystem Layers

## Layer Architecture

```
┌─────────────────────────────────────────┐
│         USER PROGRAMS                   │
│      (cat, ls, applications)            │
└─────────────────────────────────────────┘
              │ System Calls
              ▼
┌─────────────────────────────────────────┐
│    VFS (Virtual File System)            │
│    fs_node_t: name, size, inode, etc.   │
└─────────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────────┐
│       FILESYSTEM LAYER                  │
│   ext2 / FAT32 / Custom FS              │
│   (Inodes, Superblock, Bitmaps)         │
└─────────────────────────────────────────┘
              │ Read/Write sectors
              ▼
┌─────────────────────────────────────────┐
│      BLOCK DEVICE LAYER                 │
│   block_device_t: type, ops, data       │
└─────────────────────────────────────────┘
              │ Function pointers
              ▼
┌─────────────────────────────────────────┐
│       DEVICE DRIVERS                    │
│   IDE / SATA / NVME / SCSI              │
│   (Hardware-specific code)              │
└─────────────────────────────────────────┘
              │ Port I/O
              ▼
┌─────────────────────────────────────────┐
│          HARDWARE                       │
│      Physical Disk Drives               │
└─────────────────────────────────────────┘
```

## Data Flow Example

```
open("/home/hello.txt")
       ↓
VFS: Find fs_node
       ↓
ext2: Inode 42 → blocks 100-102
       ↓
Block Device: Read sectors 100-102
       ↓
IDE Driver: outb/inw to 0x1F0-0x1F7
       ↓
Hardware: Physical read
```

## Key Principle

**Abstraction**: Each layer only communicates with adjacent layers.

```
User → VFS → Filesystem → Block Device → Driver → Hardware
```

- User doesn't know about hardware
- VFS doesn't know about specific filesystems
- Filesystem doesn't know about IDE vs SATA
- Driver doesn't know about files/directories
