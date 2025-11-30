#include <ferrite/types.h>
#include <lib/string.h>

size_t strlcat(char* dst, char const* src, size_t n)
{
    size_t i = 0;

    while (dst[i] != '\0' && i < n) {
        i++;
    }

    while (*src && (i + 1) < n) {
        dst[i] = *src;
        src++;
        i++;
    }

    if (i < n) {
        dst[i] = '\0';
    }

    return i + strlen((char*)src);
}
