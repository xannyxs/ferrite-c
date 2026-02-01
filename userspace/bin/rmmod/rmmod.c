#include <libc/stdio.h>
#include <libc/syscalls.h>

int main(int argc, char* argv[])
{
    if (argc != 2) {
        printf("Usage: rmmod <module_name>\n");
        return 1;
    }

    if (delete_module(argv[1], 0) < 0) {
        printf("rmmod: failed to remove '%s'\n", argv[1]);
        return 1;
    }

    return 0;
}
