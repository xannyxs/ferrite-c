#include "sys/file/inode.h"
#include "memory/kmalloc.h"
#include "types.h"

inode_t* __alloc_inode(u16 mode)
{
    inode_t* inode = kmalloc(sizeof(inode_t));
    if (!inode) {
        return NULL;
    }

    inode->i_count = 0;
    inode->i_mode = mode;

    return inode;
}

void __dealloc_inode(void* ptr)
{
    kfree(ptr);
}
