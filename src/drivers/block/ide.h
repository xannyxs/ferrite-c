#ifndef IDE_H
#define IDE_H

#include "types.h"

typedef struct {
    u8 master; // Master (1) or slave (0)?

    u32 lba28_sectors;

    u8 supports_lba48;
    unsigned long long lba48_sectors; // If 0, lba48 is not supported

    char name[41];
} ata_drive_t;

void ide_init(void);

#endif /* IDE_H */
