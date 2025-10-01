#include "types.h"

__attribute__((target("general-regs-only"))) void*
memmove(void* dest, void const* src, size_t len)
{
    char *str_dest = dest;
    char *str_src = (char *) src;
    if (dest && src) {
        if (str_dest > str_src) {
            while (len--) {
                str_dest[len] = str_src[len];
            }
        } else {
            while (len--) {
                *str_dest = *str_src;
                str_dest++;
                str_src++;
            }
        }
    }
    return (dest);
}
