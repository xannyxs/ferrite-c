#ifndef _UAPI_UNIX_H
#define _UAPI_UNIX_H

#ifdef __KERNEL
#    include <ferrite/unix.h>
#endif /* __KERNEL */

#define AF_UNIX 1

typedef unsigned int socklen_t;

struct sockaddr_un {
    unsigned short sun_family;
    char sun_path[108];
};

#endif
