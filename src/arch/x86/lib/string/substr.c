#include "ferrite/string.h"
#include "memory/kmalloc.h"
#include <ferrite/types.h>

char* substr(char const* str, u32 start, size_t len)
{
    if (!str) {
        return NULL;
    }

    size_t slen = strlen(str);
    if (start >= slen) {
        char* empty = kmalloc(1);
        if (empty) {
            empty[0] = '\0';
        }
        return empty;
    }

    size_t available = slen - start;
    if (len > available) {
        len = available;
    }

    char* result = kmalloc(len + 1);
    if (!result) {
        return NULL;
    }

    memcpy(result, str + start, len);
    result[len] = '\0';

    return result;
}
