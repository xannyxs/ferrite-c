#include <stddef.h>

size_t strlcpy(char* dest, char const* src, size_t n)
{
    char const* src_start = src;

    if (!src || !dest)
        return 0;

    if (n > 0) {
        char* dest_end = dest + n - 1;
        while (dest < dest_end && (*dest++ = *src++))
            ;
        *dest = '\0';
    }

    while (*src++)
        ;
    return src - src_start - 1;
}
