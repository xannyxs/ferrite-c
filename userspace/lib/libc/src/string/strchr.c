#include <uapi/types.h>

char* strchr(char const* str, int c)
{
    char* ptr = (char*)str;

    while (*ptr != '\0' && *ptr != c) {
        ptr++;
    }

    return (*ptr == c) ? ptr : NULL;
}
