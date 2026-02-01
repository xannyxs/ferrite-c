#include "arch/x86/idt/syscalls.h"
#include "cpu.h"
#include "fs/mount.h"
#include "sys/process/process.h"

#include <uapi/errno.h>
#include <uapi/reboot.h>
#include <uapi/types.h>

/* General */

SYSCALL_ATTR int sys_reboot(int magic1, int magic2, unsigned int cmd, void* arg)
{
    (void)arg;

    if (magic1 != (int)FERRITE_REBOOT_MAGIC1
        || magic2 != (int)FERRITE_REBOOT_MAGIC2) {
        return -EINVAL;
    }

    if (myproc()->euid != ROOT_UID) {
        return -EPERM;
    }

    switch (cmd) {
    case FERRITE_REBOOT_CMD_RESTART:
        reboot();
        break;
    case FERRITE_REBOOT_CMD_HALT:
        halt();
        break;
    default:
        return -EINVAL;
    }

    return 0;
}

/* Mount */

SYSCALL_ATTR int sys_mount(
    char* device,
    char* dir_name,
    char* type,
    unsigned long flags,
    void* data
)
{
    (void)data;

    if (!device || !dir_name || !type) {
        return -EINVAL;
    }

    return vfs_mount(device, dir_name, type, flags);
}

SYSCALL_ATTR int sys_umount(char const* name, int flags)
{
    (void)flags;
    int ret = vfs_unmount(name);

    return ret;
}

/* UID */

SYSCALL_ATTR uid_t sys_getuid(void) { return myproc()->uid; }

SYSCALL_ATTR uid_t sys_geteuid(void) { return myproc()->euid; }

SYSCALL_ATTR s32 sys_setuid(uid_t uid)
{
    proc_t* current = myproc();

    if (current->euid == ROOT_UID) {
        current->uid = current->euid = current->suid = uid;
    } else if ((uid == current->uid) || (uid == current->suid)) {
        current->euid = uid;
    } else {
        return -EPERM;
    }

    return 0;
}

SYSCALL_ATTR s32 sys_seteuid(uid_t uid)
{
    proc_t* current = myproc();

    if (current->euid == ROOT_UID) {
        current->euid = uid;
    } else if ((uid == current->uid) || (uid == current->suid)) {
        current->euid = uid;
    } else {
        return -EPERM;
    }

    return 0;
}

SYSCALL_ATTR s32 sys_setreuid(uid_t ruid, uid_t euid)
{
    proc_t* current = myproc();
    uid_t old_ruid = current->uid;
    uid_t old_euid = current->euid;

    if (ruid != (uid_t)-1) {
        if (current->euid != ROOT_UID && ruid != old_ruid && ruid != old_euid
            && ruid != current->suid) {
            return -EPERM;
        }
    }

    if (euid != (uid_t)-1) {
        if (current->euid != ROOT_UID && euid != old_ruid && euid != old_euid
            && euid != current->suid) {
            return -EPERM;
        }
    }

    if (ruid != (uid_t)-1) {
        current->uid = ruid;
    }

    if (euid != (uid_t)-1) {
        current->euid = euid;
        if (euid != old_ruid) {
            current->suid = euid;
        }
    }

    return 0;
}

SYSCALL_ATTR s32 sys_setresuid(uid_t ruid, uid_t euid, uid_t suid)
{
    proc_t* current = myproc();

    if (current->euid != ROOT_UID) {
        if (ruid != (uid_t)-1 && ruid != current->uid && ruid != current->euid
            && ruid != current->suid) {
            return -EPERM;
        }

        if (euid != (uid_t)-1 && euid != current->uid && euid != current->euid
            && euid != current->suid) {
            return -EPERM;
        }

        if (suid != (uid_t)-1 && suid != current->uid && suid != current->euid
            && suid != current->suid) {
            return -EPERM;
        }
    }

    if (ruid != (uid_t)-1) {
        current->uid = ruid;
    }
    if (euid != (uid_t)-1) {
        current->euid = euid;
    }
    if (suid != (uid_t)-1) {
        current->suid = suid;
    }

    return 0;
}

/* GID */

SYSCALL_ATTR uid_t sys_getgid(void) { return myproc()->gid; }

SYSCALL_ATTR uid_t sys_getegid(void) { return myproc()->egid; }

SYSCALL_ATTR uid_t sys_getsgid(void) { return myproc()->sgid; }

SYSCALL_ATTR s32 sys_setgid(gid_t gid)
{
    proc_t* current = myproc();

    if (current->euid == ROOT_UID) {
        current->gid = current->egid = current->sgid = gid;
    } else if ((gid == current->gid) || (gid == current->sgid)) {
        current->egid = gid;
    } else {
        return -EPERM;
    }

    return 0;
}

SYSCALL_ATTR s32 sys_setegid(uid_t gid)
{
    proc_t* current = myproc();

    if (current->egid == ROOT_UID) {
        current->egid = gid;
    } else if ((gid == current->gid) || (gid == current->sgid)) {
        current->egid = gid;
    } else {
        return -EPERM;
    }

    return 0;
}

SYSCALL_ATTR s32 sys_setregid(gid_t rgid, gid_t egid)
{
    proc_t* current = myproc();
    gid_t old_rgid = current->gid;
    gid_t old_egid = current->egid;

    if (rgid != (gid_t)-1) {
        if (current->euid != ROOT_UID && rgid != old_rgid && rgid != old_egid
            && rgid != current->sgid) {
            return -EPERM;
        }
    }

    if (egid != (gid_t)-1) {
        if (current->euid != ROOT_UID && egid != old_rgid && egid != old_egid
            && egid != current->sgid) {
            return -EPERM;
        }
    }

    if (rgid != (gid_t)-1) {
        current->gid = rgid;
    }

    if (egid != (gid_t)-1) {
        current->egid = egid;
        if (egid != old_rgid) {
            current->sgid = egid;
        }
    }

    return 0;
}

SYSCALL_ATTR s32 sys_setresgid(gid_t rgid, gid_t egid, gid_t sgid)
{
    proc_t* current = myproc();

    if (current->euid != ROOT_UID) {
        if (rgid != (gid_t)-1 && rgid != current->gid && rgid != current->egid
            && rgid != current->sgid) {
            return -EPERM;
        }

        if (egid != (gid_t)-1 && egid != current->gid && egid != current->egid
            && egid != current->sgid) {
            return -EPERM;
        }

        if (sgid != (gid_t)-1 && sgid != current->gid && sgid != current->egid
            && sgid != current->sgid) {
            return -EPERM;
        }
    }

    if (rgid != (gid_t)-1) {
        current->gid = rgid;
    }
    if (egid != (gid_t)-1) {
        current->egid = egid;
    }
    if (sgid != (gid_t)-1) {
        current->sgid = sgid;
    }

    return 0;
}

/* GROUPS */

SYSCALL_ATTR gid_t sys_getgroups(gid_t* grouplist, int len)
{
    int i;
    int count = 0;

    for (i = 0; i < NGROUPS && myproc()->groups[i] != NOGROUP; i++) {
        count++;
    }

    if (len == 0) {
        return count;
    }

    if (len < count) {
        return -EINVAL;
    }

    for (i = 0; i < count; i++) {
        grouplist[i] = myproc()->groups[i];
    }

    return count;
}

SYSCALL_ATTR gid_t sys_setgroups(gid_t* grouplist, int len)
{
    int i;

    if (myproc()->euid != ROOT_UID) {
        return -EPERM;
    }

    if (len > NGROUPS) {
        return -EINVAL;
    }

    for (i = 0; i < len; i++, grouplist++) {
        myproc()->groups[i] = *grouplist;
    }

    if (i < NGROUPS) {
        myproc()->groups[i] = NOGROUP;
    }

    return 0;
}
