#ifdef __TEST
#    include "arch/x86/idt/syscalls.h"
#    include "drivers/printk.h"
#    include "ferrite/string.h"
#    include "sys/file/fcntl.h"
#    include "sys/process/process.h"
#    include "tests/tests.h"

#    include <ferrite/dirent.h>
#    include <ferrite/types.h>
#    include <stdbool.h>

extern u32 tests_passed;
extern u32 tests_failed;

// Syscall wrappers
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

static inline int k_unlink(char const* path)
{
    int ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_UNLINK), "b"(path)
                     : "memory");
    return ret;
}

static inline int k_chdir(char const* path)
{
    int ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_CHDIR), "b"(path)
                     : "memory");
    return ret;
}

static inline int k_getcwd(char* buf, unsigned long size)
{
    int ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_GETCWD), "b"(buf), "c"(size)
                     : "memory");
    return ret;
}

static inline int k_readdir(int fd, dirent_t* dirent, int count)
{
    int ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_READDIR), "b"(fd), "c"(dirent), "d"(count)
                     : "memory");
    return ret;
}

// Tests
TEST(fs_mkdir_basic)
{
    printk("  Creating directory /test_dir...\n");

    int result = k_mkdir("/test_dir", 0755);
    ASSERT(result == 0, "mkdir() should succeed");

    printk("  Directory created successfully\n");

    result = k_rmdir("/test_dir");
    ASSERT(result == 0, "rmdir() should succeed");

    do_exit(0);
}

TEST(fs_mkdir_trailing_slash)
{
    printk("  Testing mkdir with trailing slash...\n");

    int result = k_mkdir("/test_slash", 0755);
    ASSERT(result == 0, "mkdir without slash should succeed");

    result = k_mkdir("/test_slash/", 0755);
    printk("  mkdir with trailing slash returned: %d\n", result);
    ASSERT(
        result < 0, "mkdir with trailing slash should fail (already exists)"
    );

    result = k_rmdir("/test_slash");
    ASSERT(result == 0, "rmdir should succeed");

    do_exit(0);
}

TEST(fs_mkdir_nested)
{
    printk("  Creating nested directories...\n");

    int result = k_mkdir("/test_parent", 0755);
    ASSERT(result == 0, "parent mkdir() should succeed");

    result = k_mkdir("/test_parent/child", 0755);
    ASSERT(result == 0, "nested mkdir() should succeed");

    printk("  Nested directories created\n");

    result = k_rmdir("/test_parent/child");
    ASSERT(result == 0, "nested rmdir() should succeed");

    result = k_rmdir("/test_parent");
    ASSERT(result == 0, "parent rmdir() should succeed");

    do_exit(0);
}

TEST(fs_mkdir_already_exists)
{
    printk("  Testing mkdir on existing directory...\n");

    int result = k_mkdir("/test_exists", 0755);
    ASSERT(result == 0, "mkdir() should succeed");

    result = k_mkdir("/test_exists", 0755);
    ASSERT(result < 0, "mkdir() should fail on existing directory");

    printk("  Correctly failed on existing directory\n");

    result = k_rmdir("/test_exists");
    ASSERT(result == 0, "rmdir() should succeed");

    do_exit(0);
}

TEST(fs_rmdir_nonexistent)
{
    printk("  Testing rmdir on nonexistent directory...\n");

    int result = k_rmdir("/nonexistent_dir");
    ASSERT(result < 0, "rmdir() should fail on nonexistent directory");

    printk("  Correctly failed on nonexistent directory\n");
    do_exit(0);
}

TEST(fs_open_create)
{
    printk("  Creating file with open()...\n");

    int fd = k_open("/test_file", O_CREAT | O_RDWR, 0644);
    ASSERT(fd >= 0, "open() with O_CREAT should succeed");

    printk("  File created with fd=%d\n", fd);

    int result = k_close(fd);
    ASSERT(result == 0, "close() should succeed");

    result = k_unlink("/test_file");
    ASSERT(result == 0, "unlink() should succeed");

    do_exit(0);
}

TEST(fs_open_nonexistent)
{
    printk("  Testing open on nonexistent file...\n");

    int fd = k_open("/does_not_exist", O_RDONLY, 0);
    ASSERT(fd < 0, "open() should fail on nonexistent file");

    printk("  Correctly failed on nonexistent file\n");
    do_exit(0);
}

TEST(fs_unlink_basic)
{
    printk("  Testing unlink...\n");

    int fd = k_open("/test_unlink", O_CREAT | O_RDWR, 0644);
    ASSERT(fd >= 0, "Should create file");

    int result = k_close(fd);
    ASSERT(result == 0, "close() should succeed");

    result = k_unlink("/test_unlink");
    ASSERT(result == 0, "unlink() should succeed");

    printk("  File unlinked successfully\n");
    do_exit(0);
}

TEST(fs_unlink_nonexistent)
{
    printk("  Testing unlink on nonexistent file...\n");

    int result = k_unlink("/file_does_not_exist");
    ASSERT(result < 0, "unlink() should fail on nonexistent file");

    printk("  Correctly failed on nonexistent file\n");
    do_exit(0);
}

TEST(fs_chdir_basic)
{
    printk("  Testing chdir...\n");

    int result = k_mkdir("/test_chdir", 0755);
    ASSERT(result == 0, "mkdir() should succeed");

    result = k_chdir("/test_chdir");
    ASSERT(result == 0, "chdir() should succeed");

    char buf[256];
    result = k_getcwd(buf, sizeof(buf));
    ASSERT(result > 0, "getcwd() should succeed");
    printk("  Changed to directory: %s\n", buf);

    result = k_chdir("/");
    ASSERT(result == 0, "chdir() should succeed");

    result = k_rmdir("/test_chdir");
    ASSERT(result == 0, "rmdir() should succeed");

    do_exit(0);
}

TEST(fs_chdir_relative)
{
    printk("  Testing relative chdir...\n");

    int result = k_mkdir("/test_rel_parent", 0755);
    ASSERT(result == 0, "parent mkdir() should succeed");

    result = k_mkdir("/test_rel_parent/child", 0755);
    ASSERT(result == 0, "child mkdir() should succeed");

    result = k_chdir("/test_rel_parent");
    ASSERT(result == 0, "chdir() should succeed");

    result = k_chdir("child");
    ASSERT(result == 0, "relative chdir() should succeed");

    char buf[256];
    result = k_getcwd(buf, sizeof(buf));
    ASSERT(result > 0, "getcwd() should succeed");
    printk("  Changed to: %s\n", buf);

    result = k_chdir("/");
    ASSERT(result == 0, "chdir() should succeed");

    result = k_rmdir("/test_rel_parent/child");
    ASSERT(result == 0, "child rmdir() should succeed");

    result = k_rmdir("/test_rel_parent");
    ASSERT(result == 0, "parent rmdir() should succeed");

    do_exit(0);
}

TEST(fs_chdir_dotdot)
{
    printk("  Testing chdir with '..'...\n");

    int result = k_mkdir("/test_parent", 0755);
    ASSERT(result == 0, "parent mkdir() should succeed");

    result = k_mkdir("/test_parent/child", 0755);
    ASSERT(result == 0, "child mkdir() should succeed");

    result = k_chdir("/test_parent/child");
    ASSERT(result == 0, "chdir() should succeed");

    result = k_chdir("..");
    ASSERT(result == 0, "chdir('..') should succeed");

    char buf[256];
    result = k_getcwd(buf, sizeof(buf));
    ASSERT(result > 0, "getcwd() should succeed");
    printk("  After '..': %s\n", buf);
    ASSERT(strcmp(buf, "/test_parent") == 0, "Should be in parent directory");

    result = k_chdir("/");
    ASSERT(result == 0, "chdir() should succeed");

    result = k_rmdir("/test_parent/child");
    ASSERT(result == 0, "child rmdir() should succeed");

    result = k_rmdir("/test_parent");
    ASSERT(result == 0, "parent rmdir() should succeed");

    do_exit(0);
}

TEST(fs_chdir_nonexistent)
{
    printk("  Testing chdir to nonexistent directory...\n");

    int result = k_chdir("/does_not_exist");
    ASSERT(result < 0, "chdir() should fail on nonexistent directory");

    printk("  Correctly failed on nonexistent directory\n");
    do_exit(0);
}

TEST(fs_getcwd_basic)
{
    printk("  Testing getcwd...\n");

    char buf[256];
    int result = k_getcwd(buf, sizeof(buf));
    ASSERT(result > 0, "getcwd() should succeed");

    printk("  Current directory: %s\n", buf);
    do_exit(0);
}

TEST(fs_getcwd_after_chdir)
{
    printk("  Testing getcwd after chdir...\n");

    int result = k_mkdir("/test_getcwd", 0755);
    ASSERT(result == 0, "mkdir() should succeed");

    result = k_chdir("/test_getcwd");
    ASSERT(result == 0, "chdir() should succeed");

    char buf[256];
    result = k_getcwd(buf, sizeof(buf));
    ASSERT(result > 0, "getcwd() should succeed");
    printk("  getcwd returned: %s\n", buf);
    ASSERT(strcmp(buf, "/test_getcwd") == 0, "getcwd should match chdir");

    result = k_chdir("/");
    ASSERT(result == 0, "chdir() should succeed");

    result = k_rmdir("/test_getcwd");
    ASSERT(result == 0, "rmdir() should succeed");

    do_exit(0);
}

TEST(fs_readdir_basic)
{
    printk("  Testing readdir...\n");

    int result = k_mkdir("/test_readdir", 0755);
    ASSERT(result == 0, "mkdir() should succeed");

    int fd1 = k_open("/test_readdir/file1", O_CREAT | O_RDWR, 0644);
    ASSERT(fd1 >= 0, "open() should succeed");

    result = k_close(fd1);
    ASSERT(result == 0, "close() should succeed");

    int fd2 = k_open("/test_readdir/file2", O_CREAT | O_RDWR, 0644);
    result = k_close(fd2);
    ASSERT(result == 0, "close() should succeed");

    int dirfd = k_open("/test_readdir", O_RDONLY, 0);
    ASSERT(dirfd >= 0, "Should open directory");

    dirent_t dirent;
    int count = 0;
    int ret;
    while ((ret = k_readdir(dirfd, &dirent, 1)) > 0) {
        printk("  Entry: %s (inode=%d)\n", dirent.d_name, dirent.d_ino);
        count++;
    }

    ASSERT(ret == 0, "Dirent should return a 0");
    ASSERT(count >= 4, "Should have at least . .. file1 file2");
    printk("  Read %d directory entries\n", count);

    result = k_close(dirfd);
    ASSERT(result == 0, "close() should succeed");

    result = k_unlink("/test_readdir/file1");
    ASSERT(result == 0, "unlink() should succeed");

    result = k_unlink("/test_readdir/file2");
    ASSERT(result == 0, "unlink() should succeed");

    result = k_rmdir("/test_readdir");
    ASSERT(result == 0, "rmdir() should succeed");

    do_exit(0);
}

TEST(fs_readdir_empty)
{
    printk("  Testing readdir on empty directory...\n");

    int result = k_mkdir("/test_empty", 0755);
    ASSERT(result == 0, "mkdir() should succeed");

    int dirfd = k_open("/test_empty", O_RDONLY, 0);
    ASSERT(dirfd >= 0, "Should open directory");

    dirent_t dirent;
    int count = 0;
    while (k_readdir(dirfd, &dirent, 1) > 0) {
        count++;
    }

    ASSERT(count == 2, "Empty directory should have . and ..");
    printk("  Empty directory has %d entries (. and ..)\n", count);

    result = k_close(dirfd);
    ASSERT(result == 0, "close() should succeed");

    result = k_rmdir("/test_empty");
    ASSERT(result == 0, "rmdir() should succeed");

    do_exit(0);
}

void filesystem_tests(void)
{
    printk("\n========== FILESYSTEM TEST SUITE ==========\n\n");

    RUN_TEST(fs_mkdir_basic);
    RUN_TEST(fs_mkdir_trailing_slash);
    RUN_TEST(fs_mkdir_nested);
    RUN_TEST(fs_mkdir_already_exists);
    RUN_TEST(fs_rmdir_nonexistent);
    RUN_TEST(fs_open_create);
    RUN_TEST(fs_open_nonexistent);
    RUN_TEST(fs_unlink_basic);
    RUN_TEST(fs_unlink_nonexistent);
    RUN_TEST(fs_chdir_basic);
    RUN_TEST(fs_chdir_relative);
    RUN_TEST(fs_chdir_dotdot);
    RUN_TEST(fs_chdir_nonexistent);
    RUN_TEST(fs_getcwd_basic);
    RUN_TEST(fs_getcwd_after_chdir);
    RUN_TEST(fs_readdir_basic);
    RUN_TEST(fs_readdir_empty);

    printk("\n============================================\n");
}

#endif

typedef int _test_translation_unit_not_empty;
