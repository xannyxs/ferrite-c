#include <ferrite/string.h>
#include <ferrite/types.h>

typedef __attribute__((__may_alias__)) size_t WT;
#define WS (sizeof(WT))

void* memmove(void* dest, void const* src, size_t n)
{
    char* d = dest;
    char const* s = src;

    if (d == s) {
        return d;
    }

    if (s - d - n <= -2 * n) {
        return memcpy(d, s, n);
    }

    if (d < s) {
        if ((u32)s % WS == (u32)d % WS) {
            while ((u32)d % WS) {
                if (!n--) {
                    return dest;
                }

                *d++ = *s++;
            }

            for (; n >= WS; n -= WS, d += WS, s += WS) {
                *(WT*)d = *(WT*)s;
            }
        }

        for (; n; n--) {
            *d++ = *s++;
        }
    } else {
        if ((u32)s % WS == (u32)d % WS) {
            while ((u32)(d + n) % WS) {
                if (!n--) {
                    return dest;
                }

                d[n] = s[n];
            }

            while (n >= WS) {
                n -= WS, *(WT*)(d + n) = *(WT*)(s + n);
            }
        }

        while (n) {
            n--, d[n] = s[n];
        }
    }

    return dest;
}
