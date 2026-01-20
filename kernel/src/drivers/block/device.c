#include "drivers/block/device.h"
#include "drivers/block/ide.h"
#include "drivers/printk.h"
#include "ferrite/types.h"
#include "memory/kmalloc.h"

#include <ferrite/errno.h>
#include <limits.h>
#include <ferrite/string.h>
#include <lib/stdlib.h>

extern struct device_operations ide_device_ops;

static int num_block_devices = 0;

block_device_t* block_devices = NULL;

/* Private */

/* Public */

inline block_device_t* get_devices(void) { return block_devices; }

inline block_device_t* get_device(dev_t bdev)
{
    block_device_t* tmp = block_devices;

    while (tmp) {
        if (tmp->d_dev == bdev) {
            return tmp;
        }

        tmp = tmp->next;
    }

    return NULL;
}

block_device_t* allocate_device_slot(dev_t bdev)
{
    if (get_device(bdev)) {
        return NULL;
    }

    if (num_block_devices >= MAX_BLOCK_DEVICES) {
        printk(
            "%s: Maximum number of block devices (%d) reached\n", __func__,
            MAX_BLOCK_DEVICES
        );
        return NULL;
    }

    block_device_t* b = kmalloc(sizeof(block_device_t));
    if (!b) {
        return NULL;
    }

    memset(b, 0, sizeof(block_device_t));
    b->d_dev = bdev;

    b->next = block_devices;
    block_devices = b;

    num_block_devices += 1;

    return b;
}

void register_block_device(dev_t bdev, block_device_type_e type, void* data)
{
    if (!data) {
        printk("Cannot register device with NULL data\n");
        return;
    }

    block_device_t* d = allocate_device_slot(bdev);
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
