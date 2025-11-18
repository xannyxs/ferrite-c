#ifndef _LIMITS_H
#define _LIMITS_H

// From Linux 1.0
// #define NR_OPEN 256
// #define NGROUPS_MAX 32 /* supplemental group IDs are available */
// #define ARG_MAX 131072 /* # bytes of args + environ for exec() */
// #define CHILD_MAX 999  /* no limit :-) */
// #define OPEN_MAX 256   /* # open files a process may have */
// #define LINK_MAX 127   /* # links a file may have */
// #define MAX_CANON 255  /* size of the canonical input queue */
// #define MAX_INPUT 255  /* size of the type-ahead buffer */
// #define PATH_MAX 1024  /* # chars in a path name */
// #define PIPE_BUF 4096  /* # bytes in atomic write to a pipe */

#define MAX_INODES 256
#define NAME_MAX 255 /* Chars in a file name */
#define MAX_BLOCK_DEVICES 64

#endif
