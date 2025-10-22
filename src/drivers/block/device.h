#ifndef BLOCK_DEVICE_H
#define BLOCK_DEVICE_H

#include "types.h"

#define MAX_BLOCK_DEVICES 64

typedef enum {
    BLOCK_DEVICE_NONE = 0,
    BLOCK_DEVICE_IDE,
    BLOCK_DEVICE_SATA,
    BLOCK_DEVICE_NVME,
    BLOCK_DEVICE_UNKNOWN,
} block_device_type_e;

typedef struct {
    void* d_data;
    block_device_type_e d_type;
    struct device_operations* d_op;
} block_device_t;

struct device_operations {
    s32 (*read)(block_device_t* d, u32 lba, u32 count, void* buf, size_t len);
    s32 (*write)(
        block_device_t* d, u32 lba, u32 count, void const*, size_t len);
    void (*shutdown)(block_device_t* d);
};

block_device_t* get_device(int index);

block_device_t* get_devices(void);

void register_block_device(block_device_type_e type, void* data);

#endif /* BLOCK_DEVICE_H */
