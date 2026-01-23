#include <uapi/types.h>

size_t strlen(char const* s)
{
    size_t len = 0;

    while (s[len])
        len++;

    return len;
}
