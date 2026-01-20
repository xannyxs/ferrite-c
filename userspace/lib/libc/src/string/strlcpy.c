#include <uapi/types.h>

size_t strlcpy(char* dest, char const* src, size_t n)
{
    size_t i = 0;

    if (!src) {
        return ((unsigned long)NULL);
    }

    while (src[i] != '\0')
        i++;

    size_t j = i;
    i = 0;

    if (!n) {
        return j;
    }

    while (*src != '\0' && i < n - 1) {
        *dest = *src;
        dest++;
        src++;
        i++;
    }
    *dest = '\0';
    return j;
}
