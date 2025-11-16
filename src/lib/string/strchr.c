#include <ferrite/types.h>

char* strchr(char const* str, s32 c)
{
    char* ptr = (char*)str;

    while (*ptr != '\0' && *ptr != c) {
        ptr++;
    }

    return (*ptr == c) ? ptr : NULL;
}
