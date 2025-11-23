#ifndef BLOCK_DEVICE_H
#define BLOCK_DEVICE_H

#include <ferrite/types.h>

typedef enum {
    BLOCK_DEVICE_NONE = 0,
    BLOCK_DEVICE_IDE,
    BLOCK_DEVICE_SATA,
    BLOCK_DEVICE_NVME,
    BLOCK_DEVICE_UNKNOWN,
} block_device_type_e;

typedef struct {
    dev_t d_dev;
    u32 d_sector_size;

    void* d_data;

    block_device_type_e d_type;
    struct device_operations* d_op;
} block_device_t;

struct device_operations {
    s32 (*read)(block_device_t* d, u32 lba, u32 count, void* buf, size_t len);
    s32 (*write)(
        block_device_t* d,
        u32 lba,
        u32 count,
        void const*,
        size_t len
    );
    void (*shutdown)(block_device_t* d);
};

block_device_t* get_devices(void);

block_device_t* get_device(dev_t dev);

block_device_t* get_new_device(void);

void register_block_device(block_device_type_e type, void* data);

void root_device_init(char* cmdline);

#endif /* BLOCK_DEVICE_H */
