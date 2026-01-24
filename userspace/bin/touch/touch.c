#include <libc/stdio.h>
#include <libc/syscalls.h>
#include <uapi/fcntl.h>

int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("Usage: touch <file>\n");
        return 1;
    }

    int fd = open(argv[1], O_CREAT | O_WRONLY, 0644);
    if (fd < 0) {
        printf("touch: cannot create '%s'\n", argv[1]);
        return 1;
    }

    close(fd);
    return 0;
}
