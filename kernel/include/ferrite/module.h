#ifndef MODULE_H
#define MODULE_H

#include <types.h>

#define MOD_MAX_NAME 64

typedef enum {
    MODULE_STATE_LOADING = 0,
    MODULE_STATE_LOADED = 1,
    MODULE_STATE_UNLOADING = 2,
} module_state_e;

typedef struct module {
    char name[MOD_MAX_NAME];
    void* code_addr;
    size_t code_size;

    int (*init)(void);
    void (*cleanup)(void);

    module_state_e state;
    int ref_count;

    struct module* next;
} module_t;

void module_list(void);

#endif
