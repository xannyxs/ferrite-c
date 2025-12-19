#include <ferrite/types.h>

/* No builtin function fromg GCC of memmove */
inline void* memmove(void* dest, void const* src, size_t len)
{
    if (len == 0 || !dest || !src) {
        return dest;
    }

    u8* __d = dest;
    u8 const* __s = src;

    if (__d < __s) {
        while (len--) {
            *__d++ = *__s++;
        }
    } else if (__d > __s) {
        __d += len;
        __s += len;

        while (len--)
            *--__d = *--__s;
    }

    return dest;
}
