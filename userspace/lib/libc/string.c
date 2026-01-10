// userspace/lib/libc/string.c
// Minimal string functions

typedef unsigned int size_t;

int strlen(char const* s)
{
    int len = 0;
    while (s[len])
        len++;
    return len;
}

int strcmp(char const* a, char const* b)
{
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *a - *b;
}

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

void* memcpy(void* dst, void const* src, size_t n)
{
    char* d = dst;
    char const* s = src;
    while (n--)
        *d++ = *s++;
    return dst;
}

void* memset(void* s, int c, size_t n)
{
    unsigned char* p = s;
    while (n--)
        *p++ = (unsigned char)c;
    return s;
}
