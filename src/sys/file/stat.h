#ifndef FS_STAT_H
#define FS_STAT_H

#define S_IFSOCK 0140000 /* Socket */

#define S_IFMT 0170000 // Mask for file type

/* File type test macros */
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)

#endif /* FS_STAT_H */
