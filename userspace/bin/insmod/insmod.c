#include <libc/stdio.h>
#include <libc/string.h>
#include <libc/syscalls.h>
#include <uapi/errno.h>
#include <uapi/fcntl.h>

int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("Usage: %s <module.ko> [parameters]\n", argv[0]);
        exit(1);
    }

    char const* path = argv[1];
    char const* params = argc > 2 ? argv[2] : "";

    int fd = open(path, O_RDONLY, 0);
    if (fd < 0) {
        printf("insmod: cannot open '%s'\n", path);
        return 1;
    }

    struct stat st;
    int ret = fstat(fd, &st);
    if (ret < 0) {
        printf("insmod: fstat failed\n");
        close(fd);
        return 1;
    }

    void* buf = malloc(st.st_size);
    if (!buf) {
        printf("insmod: malloc failed\n");
        close(fd);
        return 1;
    }

    ssize_t bytes_read = read(fd, buf, st.st_size);
    if (bytes_read != st.st_size) {
        printf(
            "insmod: read failed (got %ld, expected %ld)\n", (long)bytes_read,
            (long)st.st_size
        );
        free(buf);
        close(fd);
        return 1;
    }

    printf("Loading module: %s\n", path);

    ret = init_module(buf, st.st_size, params);

    free(buf);
    close(fd);

    if (ret < 0) {
        printf("insmod: failed to load module (error %d)\n", ret);
        return 1;
    }

    printf("Module loaded successfully\n");
    return 0;
}
