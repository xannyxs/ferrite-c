#ifndef _STRING_H
#define _STRING_H

#if defined(__x86_64__) || defined(__amd64__)
#    include "string_64.h"
#elif defined(__i386__) || defined(__i686__)
#    include <arch/x86/lib/string_32.h>
#else
#    error "Unsupported x86 architecture"
#endif

#endif
