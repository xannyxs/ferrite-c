#include <uapi/types.h>

int strncmp(char const* a, char const* b, size_t n)
{
    while (n && *a && *a == *b) {
        a++;
        b++;
        n--;
    }
    if (n == 0)
        return 0;
    return *a - *b;
}
