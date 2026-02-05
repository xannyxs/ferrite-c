#ifndef _UAPI_UNIX_H
#define _UAPI_UNIX_H

#ifdef __KERNEL
#    include <ferrite/unix.h>
#endif /* __KERNEL */

#define AF_UNIX 1

struct sockaddr_un {
    unsigned short sun_family;
    char sun_path[UNIX_PATH_MAX];
};

#endif
