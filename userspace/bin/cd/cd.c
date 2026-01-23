#include <libc/stdio.h>
#include <libc/syscalls.h>

int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("Usage: cd <directory>\n");
        return -1;
    }

    int ret = chdir(argv[1]);
    if (ret < 0) {
        printf("cd: cannot change directory to '%s'\n", argv[1]);
        return -1;
    }

    return 0;
}
