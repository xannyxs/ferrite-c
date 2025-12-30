#include "drivers/block/device.h"
#include "drivers/block/ide.h"
#include "drivers/printk.h"
#include "ferrite/major.h"
#include "ferrite/types.h"

#include <ferrite/errno.h>
#include <ferrite/limits.h>
#include <ferrite/string.h>
#include <lib/stdlib.h>

extern struct device_operations ide_device_ops;

block_device_t block_devices[MAX_BLOCK_DEVICES];
block_device_t* root_device = NULL;

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

    char* type_device = strnstr(device, "hd", strlen(device));
    if (!type_device) {
        printk("Could not find device type (hd)\n");
        return result;
    }

    size_t remaining = strlen(type_device);
    if (remaining < 4) {
        printk("Device name too short. Expected format: hdXY\n");
        return result;
    }

    char drive_letter = type_device[2];
    char partition_char = type_device[3];

    if (drive_letter < 'a' || drive_letter > 'd') {
        printk("Invalid drive letter '%c'. Must be a-d\n", drive_letter);
        return result;
    }

    if (partition_char < '0' || partition_char > '9') {
        printk("Invalid partition '%c'. Must be 0-9\n", partition_char);
        return result;
    }

    s32 index = drive_letter - 'a'; // 0=hda, 1=hdb, 2=hdc, 3=hdd
    s32 major = (index < 2) ? IDE0_MAJOR : IDE1_MAJOR;
    s32 minor = partition_char - '0';

    result.valid = 1;
    result.dev = MKDEV(major, minor);
    result.drive_letter = drive_letter;
    result.partition = partition_char - '0';

    return result;
}

/* Public */

inline block_device_t* get_devices(void) { return block_devices; }

inline block_device_t* get_device(dev_t bdev)
{
    for (int i = 0; i < MAX_BLOCK_DEVICES; i++) {
        if (block_devices[i].d_dev == bdev) {
            return &block_devices[i];
        }
    }
    return NULL;
}

static block_device_t* allocate_device_slot(dev_t bdev)
{
    if (get_device(bdev)) {
        return NULL;
    }

    for (int i = 0; i < MAX_BLOCK_DEVICES; i++) {
        if (!block_devices[i].d_data) {
            block_devices[i].d_dev = bdev;
            return &block_devices[i];
        }
    }

    return NULL;
}

void register_block_device(dev_t bdev, block_device_type_e type, void* data)
{
    if (!data) {
        printk("Cannot register device with NULL data\n");
        return;
    }

    block_device_t* d = get_device(bdev);
    if (!d) {
        printk("Device %x not allocated in device table\n", bdev);
        return;
    }

    switch (type) {
    case BLOCK_DEVICE_IDE:
        d->d_sector_size = read_from_ata_data();
        d->d_op = &ide_device_ops;
        break;
    default:
        printk("Unsupported device type %d\n", type);
        return;
    }

    d->d_type = type;
    d->d_data = data;
}

int umount_device(char const* device)
{
    parsed_device_t parsed = parse_device_path(device);
    if (!parsed.valid) {
        return -EINVAL;
    }

    block_device_t* d = get_device(parsed.dev);
    if (!d) {
        printk("Device %x is not mounted\n", parsed.dev);
        return -ENODEV;
    }

    if (d == root_device) {
        printk("Cannot unmount root device\n");
        return -EBUSY;
    }

    int ret = ide_umount(parsed.dev);
    if (ret < 0) {
        return ret;
    }

    d->d_data = NULL;
    d->d_dev = 0;

    return 0;
}

int mount_device(char const* device)
{
    parsed_device_t parsed = parse_device_path(device);
    if (!parsed.valid) {
        return -EINVAL;
    }

    block_device_t* d = allocate_device_slot(parsed.dev);
    if (!d) {
        printk("Device %x already mounted or no free slots\n", parsed.dev);
        return -EBUSY;
    }

    int ret = ide_mount(parsed.dev);
    if (ret < 0) {
        d->d_data = NULL;
        d->d_dev = 0;
        return ret;
    }

    return 0;
}

// FIXME: I am assuming there wont be more than 26 devies of one type
void root_device_init(char* cmdline)
{
    char* root_param = strnstr(cmdline, "root=", strlen(cmdline));
    if (!root_param) {
        abort("No root= parameter in cmdline");
    }

    char* device_path = strchr(root_param, '/');
    if (!device_path) {
        abort("Invalid root device format. Expected: root=/dev/hdXY");
    }

    parsed_device_t parsed = parse_device_path(device_path);
    if (!parsed.valid) {
        abort("Failed to parse root device path");
    }

    block_device_t* d = allocate_device_slot(parsed.dev);
    if (!d) {
        abort("Cannot allocate root device slot");
    }

    int ret = ide_mount(parsed.dev);
    if (ret < 0) {
        abort("Failed to mount root device");
    }

    root_device = d;

    printk(
        "Root device: /dev/hd%c%d (dev=%x)\n", parsed.drive_letter,
        parsed.partition, parsed.dev
    );
}
