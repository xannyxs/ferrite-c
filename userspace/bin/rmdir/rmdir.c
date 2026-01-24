#include <libc/stdio.h>
#include <libc/syscalls.h>

int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("Usage: rmdir <directory>\n");
        return 1;
    }

    if (rmdir(argv[1]) < 0) {
        printf("rmdir: failed to remove '%s'\n", argv[1]);
        return 1;
    }

    return 0;
}
