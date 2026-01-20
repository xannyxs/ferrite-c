#include "fs/mount.h"
#include "drivers/block/device.h"
#include "drivers/printk.h"
#include <limits.h>
#include "ferrite/major.h"
#include "fs/filesystem.h"
#include "fs/stat.h"
#include "fs/vfs.h"
#include "idt/syscalls.h"
#include "memory/kmalloc.h"
#include "sys/process/process.h"

#include <ferrite/errno.h>
#include <ferrite/string.h>
#include <lib/stdlib.h>

block_device_t* root_device = NULL;
vfs_mount_t* mount_table = NULL;

/* Private */

typedef struct {
    int valid;
    dev_t dev;
    char drive_letter;
    int partition;
} parsed_device_t;

static parsed_device_t parse_device_path(char const* device)
{
    parsed_device_t result = { .valid = 0 };

    if (strncmp(device, "/dev/", 5) != 0) {
        printk("Invalid device path. Must start with /dev/\n");
        return result;
    }

    char const* type_device = device + 5;
    if (strncmp(type_device, "hd", 2) != 0) {
        printk("Unsupported device type. Expected 'hd'\n");
        return result;
    }

    char drive_letter = type_device[2];
    if (drive_letter < 'a' || drive_letter > 'd') {
        printk("Invalid drive letter '%c'. Must be a-d\n", drive_letter);
        return result;
    }

    s32 partition = 0;
    if (type_device[3] != '\0') {
        char partition_char = type_device[3];
        if (partition_char < '0' || partition_char > '9') {
            printk("Invalid partition '%c'. Must be 0-9\n", partition_char);
            return result;
        }

        if (type_device[4] != '\0') {
            printk(
                "Invalid device path. Unexpected characters after partition\n"
            );
            return result;
        }

        partition = partition_char - '0';
    }

    s32 index = drive_letter - 'a'; // 0=hda, 1=hdb, 2=hdc, 3=hdd
    s32 major = (index < 2) ? IDE0_MAJOR : IDE1_MAJOR;

    s32 base_minor = (index % 2) * 64;
    s32 minor = base_minor + partition;

    result.valid = 1;
    result.dev = MKDEV(major, minor);
    result.drive_letter = drive_letter;
    result.partition = partition;

    return result;
}

vfs_mount_t* find_mount_by_inode(vfs_inode_t* mounted_root)
{
    vfs_mount_t* tmp = mount_table;
    while (tmp) {
        vfs_inode_t* mountpoint = vfs_lookup(myproc()->root, tmp->m_name);
        if (mountpoint) {
            if (mountpoint->i_mount
                && mountpoint->i_mount->i_sb == mounted_root->i_sb
                && mountpoint->i_mount->i_ino == mounted_root->i_ino) {
                inode_put(mountpoint);
                return tmp;
            }
            inode_put(mountpoint);
        }
        tmp = tmp->next;
    }
    return NULL;
}

vfs_mount_t* find_mount_by_path(char const* path)
{
    vfs_mount_t* tmp = mount_table;

    while (tmp) {
        if (strcmp(tmp->m_name, path) == 0) {
            return tmp;
        }

        tmp = tmp->next;
    }

    return NULL;
}

/* Public */

int vfs_mount(
    char const* device,
    char const* dir_name,
    char const* type,
    unsigned long flags
)
{
    if (strlen(dir_name) >= NAME_MAX) {
        return -ENAMETOOLONG;
    }

    vfs_inode_t* mount_inode = NULL;
    if (strcmp(dir_name, "/") == 0) {
        printk("vfs_mount: Mounting root filesystem\n");
    } else {
        mount_inode = vfs_lookup(myproc()->root, dir_name);
        if (!mount_inode) {
            printk("Mount point %s does not exist\n", dir_name);
            return -ENOENT;
        }

        if (!S_ISDIR(mount_inode->i_mode)) {
            printk("Mount point %s is not a directory\n", dir_name);
            inode_put(mount_inode);
            return -ENOTDIR;
        }

        if (mount_inode->i_mount != NULL) {
            if (!(flags & MS_REMOUNT)) {
                printk("Device already mounted at %s\n", dir_name);
                inode_put(mount_inode);
                return -EBUSY;
            }

            // TODO: Handle remount (Do we really care though rn?)
            return -ENOSYS;
        }
    }

    parsed_device_t parsed = parse_device_path(device);
    if (!parsed.valid) {
        inode_put(mount_inode);
        return -EINVAL;
    }

    block_device_t* d = get_device(parsed.dev);
    if (!d) {
        printk("Device %x already mounted or no free slots\n", parsed.dev);
        inode_put(mount_inode);
        return -EBUSY;
    }

    vfs_superblock_t* sb = NULL;
    for (int i = 0; file_systems[i].name; i++) {
        if (strcmp(file_systems[i].name, type) == 0) {
            sb = kmalloc(sizeof(vfs_superblock_t));
            if (!sb) {
                inode_put(mount_inode);
                return -ENOMEM;
            }

            sb->s_dev = d->d_dev;
            sb = file_systems[i].read_super(sb, NULL, 0);
            if (sb) {
                break;
            }

            kfree(sb);
            sb = NULL;
        }
    }

    if (!sb) {
        printk("Failed to read superblock for filesystem type '%s'\n", type);

        inode_put(mount_inode);
        return -EINVAL;
    }

    vfs_mount_t* entry = kmalloc(sizeof(vfs_mount_t));
    if (!entry) {
        mount_inode->i_mount = NULL;

        inode_put(mount_inode);
        return -ENOMEM;
    }

    memcpy(entry->m_name, dir_name, NAME_MAX - 1);
    entry->m_name[NAME_MAX - 1] = '\0';
    entry->m_mount_point_inode = mount_inode;
    entry->m_sb = sb;
    entry->m_root_inode = sb->s_root_node;

    entry->next = mount_table;
    mount_table = entry;

    if (mount_inode) {
        mount_inode->i_mount = sb->s_root_node;
    }

    inode_put(mount_inode);

    return 0;
}

int vfs_unmount(char const* device)
{
    (void)device;
    return -ENOSYS;
}

void mount_root_device(char* cmdline)
{
    char* root_param = strnstr(cmdline, "root=", strlen(cmdline));
    if (!root_param) {
        abort("No root= parameter in cmdline");
    }

    char* device_path = strchr(root_param, '/');
    if (!device_path) {
        abort("Invalid root device format. Expected: root=/dev/hdXY");
    }

    char device[256];
    int i = 0;
    while (device_path[i] && device_path[i] != ' ' && i < 255) {
        device[i] = device_path[i];
        i++;
    }
    device[i] = '\0';

    parsed_device_t parsed = parse_device_path(device);
    if (!parsed.valid) {
        abort("Failed to parse root device path");
    }

    block_device_t* d = get_device(parsed.dev);
    if (!d) {
        abort("Cannot allocate root device slot");
    }

    root_device = d;

    printk("Mounting root filesystem...\n");
    int ret = sys_mount(device, "/", "ext2", 0, NULL);
    if (ret < 0) {
        abort("Failed to mount root filesystem");
    }

    printk(
        "Root device: /dev/hd%c%d (dev=%x)\n", parsed.drive_letter,
        parsed.partition, parsed.dev
    );
}
