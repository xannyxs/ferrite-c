#include <ferrite/string.h>
#include "memory/kmalloc.h"
#include <types.h>

static inline void freearray(char** s, int i)
{
    while (i > 0) {
        i--;
        kfree(s[i]);
    }
    kfree((void*)s);
}

static inline size_t countwords(char const* str, int c)
{
    s32 state = 0;
    u32 counter = 0;

    while (*str) {
        if (*str == c) {
            state = 0;
        } else if (state == 0) {
            state = 1;
            counter++;
        }
        str++;
    }

    return counter;
}

char** split(char const* s, char c)
{
    if (!s) {
        return NULL;
    }

    size_t word_count = countwords(s, c);
    char** str = (char**)kmalloc(sizeof(char*) * (word_count + 1));
    if (!str) {
        return NULL;
    }

    size_t len = 0;
    for (size_t i = 0; i < word_count; i++) {
        while (s[len] == c) {
            len++;
        }

        size_t start = len;

        while (s[len] && s[len] != c) {
            len++;
        }

        str[i] = substr(s, start, len - start);
        if (!str[i]) {
            freearray(str, (s32)i);
            return NULL;
        }
    }

    str[word_count] = NULL;
    return str;
}
