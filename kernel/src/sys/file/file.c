#include "sys/file/file.h"
#include "fs/vfs.h"
#include "memory/kmalloc.h"
#include "sys/process/process.h"

#include <uapi/errno.h>
#include <ferrite/string.h>
#include <types.h>

#include <drivers/printk.h>

int fd_alloc(void)
{
    proc_t* p = myproc();
    if (!p) {
        return -ENOENT;
    }

    for (int i = 0; i < MAX_OPEN_FILES; i += 1) {
        if (!p->open_files[i]) {
            p->open_files[i] = kmalloc(sizeof(file_t));
            if (!p->open_files[i]) {
                return -ENOMEM;
            }

            return i;
        }
    }

    return -1;
}

file_t* fd_get(s32 fd)
{
    proc_t* p = myproc();
    if (!p) {
        return NULL;
    }

    if (fd < 0 || fd > MAX_OPEN_FILES) {
        return NULL;
    }

    return p->open_files[fd];
}

void file_put(file_t* f)
{
    if (!f) {
        return;
    }

    f->f_count -= 1;
    if (f->f_count == 0) {
        if (f->f_inode) {
            inode_put(f->f_inode);
        }

        kfree(f);
    }
}
