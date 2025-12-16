#include "sys/file/file.h"
#include "fs/vfs.h"
#include "memory/kmalloc.h"
#include "sys/process/process.h"

#include <ferrite/errno.h>
#include <ferrite/types.h>
#include <lib/string.h>

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

/* Install inode in fd slot (sets f_inode, f_count=1, zeros rest) */
int fd_install(vfs_inode_t* node, file_t* file)
{
    if (!file) {
        return -1;
    }

    memset(file, 0, sizeof(file_t));
    file->f_inode = node;
    file->f_count = 1;

    return 0;
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
