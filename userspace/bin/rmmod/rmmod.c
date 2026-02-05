#include <libc/stdio.h>
#include <libc/syscalls.h>

int main(int argc, char* argv[])
{
    if (argc != 2) {
        printf("Usage: rmmod <module_name>\n");
        return 1;
    }

    int ret = delete_module(argv[1], 0);
    if (ret < 0) {
        printf("rmmod: error %d - failed to remove '%s'\n", ret, argv[1]);
        return 1;
    }

    return 0;
}
