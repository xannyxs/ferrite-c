#include <libc/stdio.h>
#include <libc/syscalls.h>
#include <uapi/types.h>

int main(void)
{
    printf("%d\n", time(NULL));

    return 0;
}
