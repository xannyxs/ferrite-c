#ifndef FS_STAT_H
#define FS_STAT_H

#define S_IFMT 0170000
#define S_IFREG 0100000
#define S_IFDIR 0040000
#define S_IFSOCK 0140000

/* File type test macros */
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)

#endif /* FS_STAT_H */
