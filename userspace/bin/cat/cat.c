#include <libc/stdio.h>
#include <libc/syscalls.h>
#include <uapi/fcntl.h>

static int cat_file(char const* filename)
{
    int fd = open(filename, O_RDONLY, 0);
    if (fd < 0) {
        printf("cat: %s: No such file or directory\n", filename);
        return 1;
    }

    char buf[4096];
    int bytes_read;

    while ((bytes_read = read(fd, buf, sizeof(buf))) > 0) {
        write(1, buf, bytes_read);
    }

    if (bytes_read < 0) {
        printf("cat: %s: Read error\n", filename);
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        char buf[4096];
        int bytes_read;
        while ((bytes_read = read(0, buf, sizeof(buf))) > 0) {
            write(1, buf, bytes_read);
        }
        return 0;
    }

    int ret = 0;
    for (int i = 1; i < argc; i++) {
        if (cat_file(argv[i]) != 0) {
            ret = 1;
        }
    }

    return ret;
}
