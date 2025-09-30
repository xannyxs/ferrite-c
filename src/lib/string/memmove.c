#include <stddef.h>

__attribute__((target("general-regs-only"))) void*
memmove(void* dest, void const* src, size_t len)
{
    char* str_dest;
    char* str_src;

    str_dest = (char*)dest;
    str_src = (char*)src;
    if (dest != NULL || src != NULL) {
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
