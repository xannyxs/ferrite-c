#include "drivers/block/device.h"
#include "drivers/block/ide.h"
#include "drivers/printk.h"
#include "ferrite/types.h"

#include <ferrite/errno.h>
#include <ferrite/limits.h>
#include <ferrite/string.h>
#include <lib/stdlib.h>

extern struct device_operations ide_device_ops;

block_device_t block_devices[MAX_BLOCK_DEVICES];

/* Private */

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

inline block_device_t* allocate_device_slot(dev_t bdev)
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
