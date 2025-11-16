#include "sys/file/file.h"
#include "lib/string.h"
#include "memory/kmalloc.h"
#include "sys/file/stat.h"
#include "sys/process/process.h"

#include <ferrite/types.h>

file_t* getfd(s32 fd)
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

file_t* file_get(void)
{
    file_t* f = kmalloc(sizeof(file_t));
    if (!f) {
        return NULL;
    }

    memset(f, 0, sizeof(file_t));

    return f;
}

void file_put(file_t* f)
{
    if (!f) {
        return;
    }

    f->f_count -= 1;
    if (f->f_count == 0) {
        if (f->f_inode) {
            // inode_put(f->f_inode);
        }

        kfree(f);
    }
}
