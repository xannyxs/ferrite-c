#include "drivers/block/device.h"
#include "drivers/block/ide.h"
#include "drivers/printk.h"

extern struct device_operations ide_device_ops;

block_device_t block_devices[MAX_BLOCK_DEVICES];

/* Public */

inline block_device_t* get_devices(void) { return block_devices; }

inline block_device_t* get_device(int index) { return &block_devices[index]; }

void register_block_device(block_device_type_e type, void* data)
{
    if (!data) {
        printk("Cannot register device with NULL data\n");
        return;
    }

    block_device_t* d = NULL;
    for (int i = 0; i < MAX_BLOCK_DEVICES; i += 1) {
        if (!block_devices[i].d_data) {
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
        d->sector_size = read_from_ata_data();
        d->d_op = &ide_device_ops;
        break;
    default:
        printk("No support for device type %d yet\n", type);
        return;
    }

    d->d_type = type;
    d->d_data = data;
}
