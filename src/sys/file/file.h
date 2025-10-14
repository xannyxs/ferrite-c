#ifndef FILE_H
#define FILE_H

#include "types.h"

#define FMODE_READ 0x01
#define FMODE_WRITE 0x02

typedef u16 mode_t;
typedef u16 dev_t;
typedef long off_t;

typedef struct file {
    struct inode* f_inode;
    struct file_operations* f_op;

    off_t f_pos;
    mode_t f_mode;
    u16 f_flags;
    u16 f_count;
} file_t;

struct file_operations {
    s32 (*read)(struct file* f, void*, size_t len);
    s32 (*write)(struct file* f, void const*, size_t len);
    s32 (*close)(struct file* f);
    off_t (*lseek)(s32 fd, off_t offset, s32 whence);
};

void __dealloc_file(void* ptr);

struct file* __alloc_file(void);

file_t* getfd(s32 fd);

#endif
