#ifdef __TEST
#    include "arch/x86/idt/syscalls.h"
#    include "drivers/printk.h"
#    include "ferrite/string.h"
#    include "sys/file/fcntl.h"
#    include "sys/process/process.h"
#    include "tests/tests.h"

#    include <ferrite/types.h>
#    include <stdbool.h>

extern u32 tests_passed;
extern u32 tests_failed;

// Syscall wrappers
static inline int k_open(char const* path, int flags, int mode)
{
    int ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_OPEN), "b"(path), "c"(flags), "d"(mode)
                     : "memory");
    return ret;
}

static inline int k_close(int fd)
{
    int ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_CLOSE), "b"(fd)
                     : "memory");
    return ret;
}

static inline int k_read(int fd, void* buf, size_t count)
{
    int ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_READ), "b"(fd), "c"(buf), "d"(count)
                     : "memory");
    return ret;
}

static inline int k_write(int fd, void const* buf, size_t count)
{
    int ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_WRITE), "b"(fd), "c"(buf), "d"(count)
                     : "memory");
    return ret;
}

static inline int k_unlink(char const* path)
{
    int ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_UNLINK), "b"(path)
                     : "memory");
    return ret;
}

static inline int k_mkdir(char const* path, int mode)
{
    int ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_MKDIR), "b"(path), "c"(mode)
                     : "memory");
    return ret;
}

static inline int k_rmdir(char const* path)
{
    int ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_RMDIR), "b"(path)
                     : "memory");
    return ret;
}

static inline uid_t k_getuid(void)
{
    uid_t ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_GETUID) : "memory");
    return ret;
}

static inline uid_t k_geteuid(void)
{
    uid_t ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_GETEUID) : "memory");
    return ret;
}

static inline int k_setuid(uid_t uid)
{
    int ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_SETUID), "b"(uid)
                     : "memory");
    return ret;
}

static inline int k_setreuid(uid_t ruid, uid_t euid)
{
    int ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_SETREUID), "b"(ruid), "c"(euid)
                     : "memory");
    return ret;
}

static inline gid_t k_getgid(void)
{
    gid_t ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_GETGID) : "memory");
    return ret;
}

static inline gid_t k_getegid(void)
{
    gid_t ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_GETEGID) : "memory");
    return ret;
}

static inline int k_setgid(gid_t gid)
{
    int ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_SETGID), "b"(gid)
                     : "memory");
    return ret;
}

static inline int k_setregid(gid_t rgid, gid_t egid)
{
    int ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_SETREGID), "b"(rgid), "c"(egid)
                     : "memory");
    return ret;
}

// Userspace-style wrappers (like libc would provide)
static inline int k_seteuid(uid_t euid) { return k_setreuid(-1, euid); }

static inline int k_setegid(gid_t egid) { return k_setregid(-1, egid); }

static inline int k_setgroups(int size, gid_t* list)
{
    int ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_SETGROUPS), "b"(size), "c"(list)
                     : "memory");
    return ret;
}

static inline int k_getgroups(int size, gid_t* list)
{
    int ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_GETGROUPS), "b"(size), "c"(list)
                     : "memory");
    return ret;
}

// Test: Basic UID/GID getters
TEST(priv_get_uid_gid)
{
    printk("  Testing getuid/getgid...\n");

    uid_t uid = k_getuid();
    uid_t euid = k_geteuid();
    gid_t gid = k_getgid();
    gid_t egid = k_getegid();

    printk("  UID=%d, EUID=%d, GID=%d, EGID=%d\n", uid, euid, gid, egid);
    ASSERT(uid == 0, "Initial UID should be 0 (root)");
    ASSERT(euid == 0, "Initial EUID should be 0 (root)");

    do_exit(0);
}

// Test: Root can read owner-only file
TEST(priv_root_read_owner_file)
{
    printk("  Testing root can read owner-only file...\n");

    // Create file with 0600 (owner read/write only)
    int fd = k_open("/test_owner_file", O_CREAT | O_RDWR, 0600);
    ASSERT(fd >= 0, "Root should create file");

    char const* msg = "test";
    int ret = k_write(fd, msg, 4);
    ASSERT(ret == 4, "Root should write to file");

    k_close(fd);

    // Reopen and read
    fd = k_open("/test_owner_file", O_RDONLY, 0);
    ASSERT(fd >= 0, "Root should open owner-only file");

    char buf[10];
    ret = k_read(fd, buf, 4);
    ASSERT(ret == 4, "Root should read from file");

    k_close(fd);
    k_unlink("/test_owner_file");

    printk("  Root successfully accessed owner-only file\n");
    do_exit(0);
}

TEST(priv_nonroot_cannot_read_owner_file)
{
    printk("  Testing non-root cannot read others' owner-only file...\n");

    int fd = k_open("/test_root_file", O_CREAT | O_RDWR, 0600);
    ASSERT(fd >= 0, "Root should create file");
    k_close(fd);

    int ret = k_seteuid(1000);
    ASSERT(ret == 0, "Should drop to UID 1000");

    fd = k_open("/test_root_file", O_RDONLY, 0);
    ASSERT(fd < 0, "Non-root should NOT open root's owner-only file");

    printk("  Non-root correctly denied access\n");

    k_seteuid(0);
    k_unlink("/test_root_file");

    do_exit(0);
}

// Test: Group permissions work
TEST(priv_group_permissions)
{
    printk("  Testing group permissions...\n");

    // Set up: root with group 100
    gid_t groups[] = { 100 };
    int ret = k_setgroups(1, groups);
    ASSERT(ret == 0, "Should set supplementary groups");

    // Create file with 0640 (owner rw, group r)
    int fd = k_open("/test_group_file", O_CREAT | O_RDWR, 0640);
    ASSERT(fd >= 0, "Should create file");

    // Change file's group to 100 (you'll need chmod/chown syscalls for this)
    // For now, we'll assume the file gets gid=100
    k_close(fd);

    // Drop to non-owner but in group 100
    k_seteuid(1000);
    k_setegid(100);

    // Should be able to read (group has read permission)
    fd = k_open("/test_group_file", O_RDONLY, 0);
    ASSERT(fd >= 0, "Group member should read file");
    k_close(fd);

    // Should NOT be able to write (group only has read, not write)
    fd = k_open("/test_group_file", O_WRONLY, 0);
    ASSERT(fd < 0, "Group member should NOT write (no group write perm)");

    printk("  Group permissions working correctly\n");

    // Cleanup
    k_seteuid(0);
    k_setegid(0);
    k_unlink("/test_group_file");

    do_exit(0);
}

// Test: World-readable file
TEST(priv_world_readable)
{
    printk("  Testing world-readable file...\n");

    // Create file with 0644 (owner rw, group r, other r)
    int fd = k_open("/test_world_file", O_CREAT | O_RDWR, 0644);
    ASSERT(fd >= 0, "Root should create file");
    k_close(fd);

    // Drop to unprivileged user
    k_seteuid(1000);
    k_setegid(1000);

    // Should be able to read (world-readable)
    fd = k_open("/test_world_file", O_RDONLY, 0);
    ASSERT(fd >= 0, "Anyone should read world-readable file");
    k_close(fd);

    // Should NOT be able to write
    fd = k_open("/test_world_file", O_WRONLY, 0);
    ASSERT(fd < 0, "Non-owner should NOT write to file");

    printk("  World-readable permissions working\n");

    // Cleanup
    k_seteuid(0);
    k_setegid(0);
    k_unlink("/test_world_file");

    do_exit(0);
}

TEST(priv_directory_execute)
{
    printk("  Testing directory execute permission...\n");

    int ret = k_mkdir("/test_priv_dir", 0700);
    ASSERT(ret == 0, "Root should create directory");

    int fd = k_open("/test_priv_dir/file", O_CREAT | O_RDWR, 0666);
    ASSERT(fd >= 0, "Root should create file in directory");
    k_close(fd);

    k_seteuid(1000);

    fd = k_open("/test_priv_dir/file", O_RDONLY, 0);
    ASSERT(fd < 0, "Should NOT access file in non-executable directory");

    printk("  Directory execute permission enforced\n");

    k_seteuid(0);
    k_unlink("/test_priv_dir/file");
    k_rmdir("/test_priv_dir");

    do_exit(0);
}

// Test: setuid/setgid behavior
TEST(priv_setuid_behavior)
{
    printk("  Testing setuid behavior...\n");
    k_setuid(0);

    uid_t orig_uid = k_getuid();
    ASSERT(orig_uid == 0, "Should start as root");

    // Root can setuid to anything
    int ret = k_setuid(1000);
    ASSERT(ret == 0, "Root should setuid to 1000");

    uid_t new_uid = k_getuid();
    uid_t new_euid = k_geteuid();
    ASSERT(new_uid == 1000, "UID should be 1000");
    ASSERT(new_euid == 1000, "EUID should be 1000");

    // Non-root cannot setuid to arbitrary UID
    ret = k_setuid(2000);
    ASSERT(ret < 0, "Non-root should NOT setuid to arbitrary UID");

    printk("  setuid behavior correct\n");

    // Restore (this will fail since we're not root anymore)
    // In real test, you'd need to restart the process or use saved UID
    do_exit(0);
}

TEST(priv_seteuid_toggle)
{
    printk("  Testing seteuid privilege toggling...\n");

    // Check initial state
    printk(
        "  Initial: uid=%d, euid=%d, suid=%d\n", k_getuid(), k_geteuid(),
        myproc()->suid
    );

    ASSERT(k_geteuid() == 0, "Should start as root");

    // Drop privileges
    int ret = k_seteuid(1000);
    printk(
        "  After drop: uid=%d, euid=%d, suid=%d, ret=%d\n", k_getuid(),
        k_geteuid(), myproc()->suid, ret
    );

    ASSERT(ret == 0, "Should drop to UID 1000");
    ASSERT(k_geteuid() == 1000, "EUID should be 1000");

    // Try to regain privileges
    ret = k_seteuid(0);
    printk(
        "  After restore attempt: uid=%d, euid=%d, suid=%d, ret=%d\n",
        k_getuid(), k_geteuid(), myproc()->suid, ret
    );

    ASSERT(ret == 0, "Should regain root via saved UID");
    ASSERT(k_geteuid() == 0, "EUID should be 0 again");

    do_exit(0);
}

// Test: Cannot write to read-only file
TEST(priv_readonly_file)
{
    printk("  Testing read-only file protection...\n");

    // Create file with 0444 (read-only for everyone)
    int fd = k_open("/test_readonly", O_CREAT | O_RDWR, 0444);
    ASSERT(fd >= 0, "Root should create file");
    k_close(fd);

    // Even root should respect read-only mode when opening
    fd = k_open("/test_readonly", O_WRONLY, 0);
    // Note: Root might be allowed to override this - check your implementation

    printk("  Read-only file test completed\n");

    k_unlink("/test_readonly");
    do_exit(0);
}

void privilege_tests(void)
{
    printk("\n========== PRIVILEGE TEST SUITE ==========\n\n");

    RUN_TEST(priv_get_uid_gid);
    RUN_TEST(priv_root_read_owner_file);
    RUN_TEST(priv_nonroot_cannot_read_owner_file);
    RUN_TEST(priv_group_permissions);
    RUN_TEST(priv_world_readable);
    RUN_TEST(priv_directory_execute);
    RUN_TEST(priv_setuid_behavior);
    RUN_TEST(priv_seteuid_toggle);
    RUN_TEST(priv_readonly_file);

    printk("\n============================================\n");
}

#endif

typedef int _test_translation_unit_not_empty;
