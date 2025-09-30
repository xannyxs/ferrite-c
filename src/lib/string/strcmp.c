#include <stddef.h>

int strcmp(char const* s1, char const* s2)
{
    size_t i = 0;

    while ((s1[i] && s2[i]) && (s1[i] == s2[i])) {
        i++;
    }
    return s1[i] - s2[i];
}
