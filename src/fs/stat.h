#ifndef _STAT_H
#define _STAT_H

#include <ferrite/types.h>

struct stat {
    u16 st_dev;
    u16 __pad1;
    unsigned long st_ino;
    u16 st_mode;
    u16 st_nlink;
    u16 st_uid;
    u16 st_gid;
    u16 st_rdev;
    u16 __pad2;
    unsigned long st_size;
    unsigned long st_blksize;
    unsigned long st_blocks;
    unsigned long st_atime;
    unsigned long __unused1;
    unsigned long st_mtime;
    unsigned long __unused2;
    unsigned long st_ctime;
    unsigned long __unused3;
    unsigned long __unused4;
    unsigned long __unused5;
};

#define S_IFMT 0170000
#define S_IFREG 0100000
#define S_IFDIR 0040000
#define S_IFSOCK 0140000

#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)

#endif
