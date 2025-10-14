#include "sys/file/file.h"
#include "lib/string.h"
#include "memory/kmalloc.h"
#include "sys/process/process.h"
#include "types.h"

file_t* getfd(s32 fd)
{
    proc_t* p = myproc();
    if (!p) {
        return NULL;
    }

    return p->open_files[fd];
}

file_t* __alloc_file(void)
{
    file_t* f = kmalloc(sizeof(file_t));
    if (!f) {
        return NULL;
    }

    memset(f, 0, sizeof(file_t));

    return f;
}

void __dealloc_file(void* ptr)
{
    kfree(ptr);
}
