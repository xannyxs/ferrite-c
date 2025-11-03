#include "types.h"

s32 strncmp(char const* str1, char const* str2, size_t n)
{
    if (n == 0) {
        return 0;
    }

    size_t i = 0;

    for (size_t i = 0;
         str1[i] != 0 && str2[i] != 0 && (str1[i] == str2[i] && n - 1 > i);
         i += 1) { }

    return ((unsigned char)str1[i] - (unsigned char)str2[i]);
}
