#ifndef IDE_H
#define IDE_H

#include <ferrite/types.h>

#define MAX_IDE_CONTR 2

typedef struct {
    u8 controller; // 0 = IDE0, 1 = IDE1
    u8 drive;      // 0 = Master, 1 = Slave

    u32 lba28_sectors;
    u8 supports_lba48;
    unsigned long long lba48_sectors;

    char name[41];
} ata_drive_t;

typedef struct ide_controller {
    u16 base_port;
    u16 ctrl_port;
    u8 irq;

    int present;
} ide_controller_t;

int ide_detach(dev_t bdev); // Unregister and cleanup device

s32 ide_probe(dev_t); // Check if device is valid

u32 read_from_ata_data(void);

void ide_init(void);

#endif /* IDE_H */
