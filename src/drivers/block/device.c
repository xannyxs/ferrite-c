#include "drivers/block/device.h"
#include "drivers/block/ide.h"
#include "drivers/printk.h"
#include "ferrite/major.h"

#include <ferrite/limits.h>
#include <lib/stdlib.h>
#include <lib/string.h>

extern struct device_operations ide_device_ops;

block_device_t block_devices[MAX_BLOCK_DEVICES];

/* Private */

/* Public */

inline block_device_t* get_device(dev_t dev) { return &block_devices[dev]; }

void register_block_device(block_device_type_e type, void* data)
{
    if (!data) {
        printk("Cannot register device with NULL data\n");
        return;
    }

    block_device_t* d = NULL;
    for (int i = 0; i < MAX_BLOCK_DEVICES; i += 1) {
        if (!block_devices[i].d_data) {
            block_devices[i].d_dev = i;
            d = &block_devices[i];
            break;
        }
    }

    if (!d) {
        printk("All device blocks are taken\n");
        return;
    }

    switch (type) {
    case BLOCK_DEVICE_IDE:
        d->d_sector_size = read_from_ata_data();
        d->d_op = &ide_device_ops;
        break;
    default:
        printk("No support for device type %d yet\n", type);
        return;
    }

    d->d_type = type;
    d->d_data = data;
}

// FIXME: I am assuming there wont be more than 26 devies of one type
void root_device_init(char* cmdline)
{
    char* root_device_line = strnstr(cmdline, "root=", 32);
    char* device = strrchr(root_device_line, '/');
    char* type_device = strnstr(device, "hd", 12);

    s32 major = (type_device[2] - 'a' == 0) ? IDE0_MAJOR : IDE1_MAJOR;
    s32 minor = type_device[3] - '0';

    if (strncmp(type_device, "hd", 2) != 0) {
        abort("No root device");
    }

    if (ide_mount(major, minor) < 0) {
        abort("Failed to mount root device");
    }
}
