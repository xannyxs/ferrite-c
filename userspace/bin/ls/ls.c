#include <libc/stdio.h>
#include <libc/string.h>
#include <libc/syscalls.h>
#include <uapi/dirent.h>
#include <uapi/stat.h>

int main(char const* path)
{
    int ret = 1;
    char buf[512];
    long size = 512;

    if (!path) {
        getcwd(buf, size);
    }

    char const* dir = path ? path : (char*)buf;

    int fd = open(dir, 0, 0);
    if (fd < 0) {
        printf("Could not open dir\n");
        return -1;
    }

    dirent_t dirent = { 0 };

    while (1) {
        ret = readdir(fd, &dirent, 1);
        if (ret <= 0) {
            break;
        }

        char fullpath[256];
        strlcpy(fullpath, dir, sizeof(fullpath));

        size_t len = strlen(fullpath);
        if (len > 0 && fullpath[len - 1] != '/') {
            fullpath[len] = '/';
            fullpath[len + 1] = '\0';
        }

        strlcat(fullpath, (char*)dirent.d_name, sizeof(fullpath));

        struct stat st;
        int stat_ret;
        stat_ret = stat(fullpath, &st);

        if (stat_ret == 0) {
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
            printf(
                " %2d %8ld %10lu %s\n", st.st_nlink, st.st_size, st.st_mtime,
                (char*)dirent.d_name
            );
        } else {
            printf("?????????? ?? ???????? %s\n", (char*)dirent.d_name);
        }
    }

    if (ret < 0) {
        printf("Something went wrong reading dir\n");
    }

    __asm__ volatile("int $0x80" : "=a"(fd) : "a"(6), "b"(fd) : "memory");
}
