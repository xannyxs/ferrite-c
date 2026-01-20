#ifndef _DIRENT_H
#define _DIRENT_H

#include <limits.h>
#include <types.h>

typedef struct {
    long d_ino;
    off_t d_off;
    unsigned short d_reclen;
    char d_name[NAME_MAX + 1];
} dirent_t;

#endif
