typedef unsigned int size_t;

extern int write(int fd, void const* buf, size_t count);

int strlen(char const* s)
{
    int len = 0;
    while (s[len])
        len++;
    return len;
}

int strcmp(char const* s1, char const* s2)
{
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
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
