#include "drivers/block/device.h"
#include "drivers/block/ide.h"
#include "drivers/printk.h"
#include "ferrite/major.h"

#include <ferrite/limits.h>
#include <ferrite/string.h>
#include <lib/stdlib.h>

extern struct device_operations ide_device_ops;

block_device_t block_devices[MAX_BLOCK_DEVICES];

/* Private */

/* Public */

inline block_device_t* get_devices(void) { return block_devices; }

inline block_device_t* get_device(dev_t dev) { return &block_devices[dev]; }

inline block_device_t* get_new_device(void)
{
    for (int i = 0; i < MAX_BLOCK_DEVICES; i += 1) {
        if (!block_devices[i].d_data) {
            block_devices[i].d_dev = i;
            return &block_devices[i];
        }
    }

    return NULL;
}

void register_block_device(block_device_type_e type, void* data)
{
    if (!data) {
        printk("Cannot register device with NULL data\n");
        return;
    }

    block_device_t* d = get_new_device();
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
    if (!root_device_line) {
        abort("No root= parameter in cmdline");
    }

    char* device = strrchr(root_device_line, '/');
    if (!device) {
        abort("Invalid root device format");
    }

    char* type_device = strnstr(device, "hd", 12);
    if (!type_device || strncmp(type_device, "hd", 2) != 0) {
        abort("No root device");
    }

    s32 index = type_device[2] - 'a'; // 0=a, 1=b, 2=c, 3=d
    s32 major = (index < 2) ? IDE0_MAJOR : IDE1_MAJOR;

    s32 minor = type_device[3] - '0';

    if (ide_mount(major, minor) < 0) {
        abort("Failed to mount root device");
    }
}
