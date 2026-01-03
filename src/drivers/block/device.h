#ifndef BLOCK_DEVICE_H
#define BLOCK_DEVICE_H

#include <ferrite/types.h>

#define MAJOR(a) (int)((unsigned short)(a) >> 8)
#define MINOR(a) (int)((unsigned short)(a) & 0xFF)
#define MKDEV(a, b) ((int)((((a) & 0xff) << 8) | ((b) & 0xff)))

typedef enum {
    BLOCK_DEVICE_NONE = 0,
    BLOCK_DEVICE_IDE,
    BLOCK_DEVICE_SATA,
    BLOCK_DEVICE_NVME,
    BLOCK_DEVICE_UNKNOWN,
} block_device_type_e;

typedef struct block_device {
    dev_t d_dev;
    u32 d_sector_size;

    void* d_data;

    block_device_type_e d_type;
    struct device_operations* d_op;

    struct block_device* next;
} block_device_t;

struct device_operations {
    s32 (*read)(block_device_t*, u32 lba, u32 count, void*, size_t len);
    s32 (*write)(block_device_t*, u32 lba, u32 count, void const*, size_t len);
    void (*shutdown)(block_device_t*);
};

block_device_t* get_devices(void);

block_device_t* get_device(dev_t);

block_device_t* allocate_device_slot(dev_t);

void register_block_device(dev_t, block_device_type_e, void*);

#endif /* BLOCK_DEVICE_H */
