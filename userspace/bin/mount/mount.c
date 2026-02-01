#include <libc/stdio.h>
#include <libc/stdlib.h>
#include <libc/string.h>
#include <libc/syscalls.h>

int main(int argc, char* argv[])
{
    if (argc < 3) {
        printf("Usage: mount <device> <mountpoint> [fstype] [flags]\n");
        printf("Example: mount /dev/sda1 /mnt ext2\n");
        return 1;
    }

    char* device = argv[1];
    char* mountpoint = argv[2];
    char* fstype = argc > 3 ? argv[3] : "ext2";
    unsigned long flags = argc > 4 ? atoi(argv[4]) : 0;

    int ret = mount(device, mountpoint, fstype, flags, NULL);

    if (ret < 0) {
        printf(
            "mount: failed to mount %s on %s (error %d)\n", device, mountpoint,
            ret
        );
        return 1;
    }

    printf("mount: successfully mounted %s on %s\n", device, mountpoint);
    return 0;
}
