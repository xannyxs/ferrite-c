#include "fs/vfs.h"
#include "fs/mount.h"
#include "lib/stdlib.h"

#include <ferrite/string.h>
#include <ferrite/types.h>

vfs_inode_t* root_inode = NULL;

void vfs_init(void)
{
    vfs_mount_t* root_mount = find_mount("/");
    if (!root_mount) {
        abort("Root filesystem not mounted");
    }

    root_inode = root_mount->m_root_inode;
    if (!root_inode) {
        abort("Root inode is NULL");
    }
}
