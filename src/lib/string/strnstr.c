#include "types.h"

char* strnstr(char const* str, char const* to_find, size_t len)
{
    size_t pos = 0;

    if (!*to_find) {
        return (char*)str;
    }

    while (str[pos] && pos < len) {
        if (str[pos] == to_find[0]) {
            size_t i = 1;

            while (to_find[i] && str[pos + i] == to_find[i] && (pos + i) < len
            ) {
                i += 1;
            }

            if (!to_find[i]) {
                return (char*)&str[pos];
            }
        }
        pos++;
    }

    return NULL;
}
