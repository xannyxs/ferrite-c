#ifndef IDE_H
#define IDE_H

#include <ferrite/types.h>

typedef struct {
    u8 drive; // Master (0) / Slave (1)

    u32 lba28_sectors;
    u8 supports_lba48;
    unsigned long long lba48_sectors;

    char name[41];
} ata_drive_t;

s32 ide_umount(dev_t);

s32 ide_mount(dev_t);

u32 read_from_ata_data(void);

#endif /* IDE_H */
