#ifndef ENTRY_H
#define ENTRY_H

#include <stdint.h>

typedef struct entry {
    uint16_t limit;
    uint16_t lower_base;
    uint8_t middle_base;
    uint8_t access;
    uint8_t flags;
    uint8_t higher_base;
} __attribute__((packed)) entry_t;

typedef struct descriptor {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) descriptor_pointer_t;

#endif /* ENTRY_H */
