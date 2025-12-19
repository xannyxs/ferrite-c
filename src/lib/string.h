#ifndef STRING_H
#define STRING_H

#include <ferrite/types.h>

static inline size_t strlen(char const* s)
{
    int d0;
    register int __res __asm__("cx");

    __asm__(
        "cld\n\t"   // Clear Direction Flag: scan forward (increment RDI)
        "repne\n\t" // REPeat while Not Equal: loop while byte != AL and RCX !=
                    // 0
        "scasb\n\t" // SCAn String Byte: compare [RDI] with AL, inc RDI, dec RCX
        "notl %0\n\t" // Bitwise NOT RCX: convert "remaining" to "done" count
        "decl %0\n\t" // Decrement RCX: subtract 1 to exclude the '\0' byte

        : "=c"(__res),
          "=&D"(d0) // Outputs: RCX→__res (length), RDI→d0 (final ptr)

        : "1"(s), // Input: s → RDI (same register as output #1)
          "a"(0), // Input: 0 → AL (searching for null byte)
          "0"(0xffffffff)
    ); // Input: 0xffffffff → RCX (max count, same as output #0)

    return __res;
}

static inline char* strchr(char const* str, s32 c)
{
    register char* __res __asm__("ax");

    __asm__("cld\n\t"
            "movb %%al, %%ah\n"
            "1:\tlodsb\n\t"
            "cmpb %%ah, %%al\n\t"
            "je 2f\n\t"
            "test %%al, %%al\n\t"
            "jne 1b\n\t"
            "movl $1,%1\n"
            "2:\tmovl %1,%0\n\t"
            "decl %0"

            : "=a"(__res)
            : "S"(str), "0"(c));

    return __res;
}

static inline char* strrchr(char const* str, s32 c)
{
    register char* __res __asm__("dx");

    __asm__("cld\n\t"
            "movb %%al, %%ah\n"
            "1:\tlodsb\n\t"

            "cmpb %%ah, %%al\n\t"
            "je 2f\n\t"

            "movl %%esi, %0\n\t"
            "decl %0\n"

            "2:\ttestb %%al, %%al\n\t"
            "jne 1b\n\t"

            : "=a"(__res)
            : "S"(str), "0"(c));

    return __res;
}

static inline void* memcpy(void* dest, void const* src, size_t n)
{
    __asm__("cld\n\t"
            "movl %%edx, %%ecx\n\t"
            "shrl $2,%%ecx\n\t"
            "rep ; movsl\n\t"
            "testb $1,%%dl\n\t"
            "je 1f\n\t"
            "movsb\n"
            "1:\ttestb $2,%%dl\n\t"
            "je 2f\n\t"
            "movsw\n"
            "2:\n"

            : /* No output */
            : "d"(n), "D"((long)dest), "S"((long)src)
            : "memory");

    return dest;
}

void* memmove(void* dest, void const* src, size_t len);

void memset(void* s, int c, size_t n);

s32 strcmp(char const* s1, char const* s2);

s32 strncmp(char const* str1, char const* str2, size_t n);

char** split(char const* s, char c);

char* substr(char const* str, u32 start, size_t len);

size_t strlcpy(char* dest, char const* src, size_t n);

char* strnstr(char const* str, char const* to_find, size_t len);

size_t strlcat(char* dst, char const* src, size_t n);

#endif /* STRING_H */
