#include "types.h"

char* strrchr(char const* str, s32 c)
{
    char* ptr = (char*)str;
    char* last = NULL;

    while (*ptr != '\0') {
        if (*ptr == c) {
            last = ptr;
        }

        ptr++;
    }

    if (*ptr == c) {
        last = ptr;
    }

    return last;
}
