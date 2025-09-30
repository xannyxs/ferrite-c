#ifndef ENTRY_H
#define ENTRY_H

#include "types.h"

typedef struct entry {
    u16 limit;
    u16 lower_base;
    u8 middle_base;
    u8 access;
    u8 flags;
    u8 higher_base;
} __attribute__((packed)) entry_t;

typedef struct descriptor {
    u16 limit;
    u32 base;
} __attribute__((packed)) descriptor_pointer_t;

#endif /* ENTRY_H */
