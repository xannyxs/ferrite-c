#include <libc/stdio.h>
#include <libc/string.h>
#include <libc/syscalls.h>
#include <uapi/dirent.h>
#include <uapi/stat.h>

int main(int argc, char** argv)
{
    char buf[512];
    char const* path = NULL;

    if (argc > 1) {
        path = argv[1];
    }

    if (!path) {
        getcwd(buf, sizeof(buf));
        path = buf;
    }

    int fd = open(path, 0, 0);
    if (fd < 0) {
        printf("ls: cannot open '%s'\n", path);
        return 1;
    }

    dirent_t dirent;
    int ret;

    while ((ret = readdir(fd, &dirent, 1)) > 0) {

        char fullpath[256];
        (void)snprintf(
            fullpath, sizeof(fullpath), "%s%s%s", path,
            (path[strlen(path) - 1] == '/') ? "" : "/", dirent.d_name
        );

        struct stat st;
        if (stat(fullpath, &st) == 0) {
            printf("%c", S_ISDIR(st.st_mode) ? 'd' : '-');
            printf("%c", (st.st_mode & S_IRUSR) ? 'r' : '-');
            printf("%c", (st.st_mode & S_IWUSR) ? 'w' : '-');
            printf("%c", (st.st_mode & S_IXUSR) ? 'x' : '-');
            printf("%c", (st.st_mode & S_IRGRP) ? 'r' : '-');
            printf("%c", (st.st_mode & S_IWGRP) ? 'w' : '-');
            printf("%c", (st.st_mode & S_IXGRP) ? 'x' : '-');
            printf("%c", (st.st_mode & S_IROTH) ? 'r' : '-');
            printf("%c", (st.st_mode & S_IWOTH) ? 'w' : '-');
            printf("%c", (st.st_mode & S_IXOTH) ? 'x' : '-');
            printf(" %2d %8ld %s\n", st.st_nlink, st.st_size, dirent.d_name);
        } else {
            printf("?????????? ?? ???????? %s\n", dirent.d_name);
        }
    }

    close(fd);
    return 0;
}
