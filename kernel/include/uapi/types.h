#ifndef TYPES_H
#define TYPES_H

#ifdef __KERNEL
#    include <ferrite/types.h>
#endif /* __KERNEL */

#define NULL ((void*)0)

typedef long off_t;
typedef unsigned short dev_t;
typedef unsigned short uid_t;
typedef unsigned short gid_t;

typedef unsigned int size_t;
typedef unsigned long long time_t;

#endif
