#ifndef SOCKET_H
#define SOCKET_H

#ifdef __KERNEL
#    include <ferrite/socket.h>
#endif /* __KERNEL */

#define SOCK_STREAM 1 /* Stream socket (TCP, Unix STREAM) */
#define SOCK_DGRAM 2  /* Datagram socket (UDP, Unix DGRAM) */

#endif
