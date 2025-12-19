#include <ferrite/types.h>

s32 strncmp(char const* _l, char const* _r, size_t n)
{
    u8 const* l = (void*)_l;
    u8 const* r = (void*)_r;

    if (!n--) {
        return 0;
    }

    for (; *l && *r && n && *l == *r; l++, r++, n--)
        ;

    return *l - *r;
}
