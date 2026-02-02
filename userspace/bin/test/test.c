#include <libc/stdio.h>
#include <libc/string.h>
#include <libc/syscalls.h>
#include <uapi/dirent.h>
#include <uapi/fcntl.h>
#include <uapi/types.h>

#include <stdbool.h>

#define ASSERT(cond, msg)                                      \
    if (!(cond)) {                                             \
        printf("[FAIL] %s:%d: %s\n", __FILE__, __LINE__, msg); \
        exit(1);                                               \
    }

#define ASSERT_EQ(a, b, message) ASSERT((a) == (b), message)

#define TEST(name) static void __attribute__((unused)) test_##name(void)

#define RUN_TEST(name)                  \
    do {                                \
        printf("[TEST] " #name "... "); \
                                        \
        test_##name();                  \
        printf("PASS\n");               \
        tests_passed++;                 \
    } while (0)

int tests_passed = 0;
int tests_failed = 0;

TEST(fs_mkdir_basic)
{
    printf("  Creating directory /test_dir...\n");

    int result = mkdir("/test_dir", 0755);
    ASSERT(result == 0, "mkdir() should succeed");

    printf("  Directory created successfully\n");

    result = rmdir("/test_dir");
    ASSERT(result == 0, "rmdir() should succeed");
}

TEST(fs_mkdir_trailing_slash)
{
    printf("  Testing mkdir with trailing slash...\n");

    int result = mkdir("/test_slash/", 0755);
    printf("  mkdir with trailing slash returned: %d\n", result);
    ASSERT(result == 0, "mkdir with trailing slash should work");

    result = rmdir("/test_slash/");
    ASSERT(result == 0, "rmdir with trailing slash should work");

    result = mkdir("///////////test_slash///////////", 0755);
    printf("  mkdir with trailing slash returned: %d\n", result);
    ASSERT(result == 0, "mkdir with trailing slash should work");

    result = rmdir("///////////test_slash///////////");
    ASSERT(result == 0, "rmdir with trailing slash should work");
}

TEST(fs_mkdir_nested)
{
    printf("  Creating nested directories...\n");

    int result = mkdir("/test_parent", 0755);
    ASSERT(result == 0, "parent mkdir() should succeed");

    result = mkdir("/test_parent/child", 0755);
    ASSERT(result == 0, "nested mkdir() should succeed");

    printf("  Nested directories created\n");

    result = rmdir("/test_parent/child");
    ASSERT(result == 0, "nested rmdir() should succeed");

    result = rmdir("/test_parent");
    ASSERT(result == 0, "parent rmdir() should succeed");
}

TEST(fs_mkdir_already_exists)
{
    printf("  Testing mkdir on existing directory...\n");

    int result = mkdir("/test_exists", 0755);
    ASSERT(result == 0, "mkdir() should succeed");

    result = mkdir("/test_exists", 0755);
    ASSERT(result < 0, "mkdir() should fail on existing directory");

    printf("  Correctly failed on existing directory\n");

    result = rmdir("/test_exists");
    ASSERT(result == 0, "rmdir() should succeed");
}

TEST(fs_rmdir_nonexistent)
{
    printf("  Testing rmdir on nonexistent directory...\n");

    int result = rmdir("/nonexistent_dir");
    ASSERT(result < 0, "rmdir() should fail on nonexistent directory");

    printf("  Correctly failed on nonexistent directory\n");
}

TEST(fs_open_create)
{
    printf("  Creating file with open()...\n");

    int fd = open("/test_file", O_CREAT | O_RDWR, 0644);
    ASSERT(fd >= 0, "open() with O_CREAT should succeed");

    printf("  File created with fd=%d\n", fd);

    int result = close(fd);
    ASSERT(result == 0, "close() should succeed");

    result = unlink("/test_file");
    ASSERT(result == 0, "unlink() should succeed");
}

TEST(fs_open_nonexistent)
{
    printf("  Testing open on nonexistent file...\n");

    int fd = open("/does_not_exist", O_RDONLY, 0);
    ASSERT(fd < 0, "open() should fail on nonexistent file");

    printf("  Correctly failed on nonexistent file\n");
}

TEST(fs_unlink_basic)
{
    printf("  Testing unlink...\n");

    int fd = open("/test_unlink", O_CREAT | O_RDWR, 0644);
    ASSERT(fd >= 0, "Should create file");

    int result = close(fd);
    ASSERT(result == 0, "close() should succeed");

    result = unlink("/test_unlink");
    ASSERT(result == 0, "unlink() should succeed");

    printf("  File unlinked successfully\n");
}

TEST(fs_unlink_nonexistent)
{
    printf("  Testing unlink on nonexistent file...\n");

    int result = unlink("/file_does_not_exist");
    ASSERT(result < 0, "unlink() should fail on nonexistent file");

    printf("  Correctly failed on nonexistent file\n");
}

TEST(fs_chdir_basic)
{
    printf("  Testing chdir...\n");

    int result = mkdir("/test_chdir", 0755);
    ASSERT(result == 0, "mkdir() should succeed");

    result = chdir("/test_chdir");
    ASSERT(result == 0, "chdir() should succeed");

    char buf[256];
    result = getcwd(buf, sizeof(buf));
    ASSERT(result > 0, "getcwd() should succeed");
    printf("  Changed to directory: %s\n", buf);

    result = chdir("/");
    ASSERT(result == 0, "chdir() should succeed");

    result = rmdir("/test_chdir");
    ASSERT(result == 0, "rmdir() should succeed");
}

TEST(fs_chdir_relative)
{
    printf("  Testing relative chdir...\n");

    int result = mkdir("/test_rel_parent", 0755);
    ASSERT(result == 0, "parent mkdir() should succeed");

    result = mkdir("/test_rel_parent/child", 0755);
    ASSERT(result == 0, "child mkdir() should succeed");

    result = chdir("/test_rel_parent");
    ASSERT(result == 0, "chdir() should succeed");

    result = chdir("child");
    ASSERT(result == 0, "relative chdir() should succeed");

    char buf[256];
    result = getcwd(buf, sizeof(buf));
    ASSERT(result > 0, "getcwd() should succeed");
    printf("  Changed to: %s\n", buf);

    result = chdir("/");
    ASSERT(result == 0, "chdir() should succeed");

    result = rmdir("/test_rel_parent/child");
    ASSERT(result == 0, "child rmdir() should succeed");

    result = rmdir("/test_rel_parent");
    ASSERT(result == 0, "parent rmdir() should succeed");
}

TEST(fs_chdir_dotdot)
{
    printf("  Testing chdir with '..'...\n");

    int result = mkdir("/test_parent", 0755);
    ASSERT(result == 0, "parent mkdir() should succeed");

    result = mkdir("/test_parent/child", 0755);
    ASSERT(result == 0, "child mkdir() should succeed");

    result = chdir("/test_parent/child");
    ASSERT(result == 0, "chdir() should succeed");

    result = chdir("..");
    ASSERT(result == 0, "chdir('..') should succeed");

    char buf[256];
    result = getcwd(buf, sizeof(buf));
    ASSERT(result > 0, "getcwd() should succeed");
    printf("  After '..': %s\n", buf);
    ASSERT(strcmp(buf, "/test_parent") == 0, "Should be in parent directory");

    result = chdir("/");
    ASSERT(result == 0, "chdir() should succeed");

    result = rmdir("/test_parent/child");
    ASSERT(result == 0, "child rmdir() should succeed");

    result = rmdir("/test_parent");
    ASSERT(result == 0, "parent rmdir() should succeed");
}

TEST(fs_chdir_nonexistent)
{
    printf("  Testing chdir to nonexistent directory...\n");

    int result = chdir("/does_not_exist");
    ASSERT(result < 0, "chdir() should fail on nonexistent directory");

    printf("  Correctly failed on nonexistent directory\n");
}

TEST(fs_getcwd_basic)
{
    printf("  Testing getcwd...\n");

    char buf[256];
    int result = getcwd(buf, sizeof(buf));
    ASSERT(result > 0, "getcwd() should succeed");

    printf("  Current directory: %s\n", buf);
}

TEST(fs_getcwd_after_chdir)
{
    printf("  Testing getcwd after chdir...\n");

    int result = mkdir("/test_getcwd", 0755);
    ASSERT(result == 0, "mkdir() should succeed");

    result = chdir("/test_getcwd");
    ASSERT(result == 0, "chdir() should succeed");

    char buf[256];
    result = getcwd(buf, sizeof(buf));
    ASSERT(result > 0, "getcwd() should succeed");
    printf("  getcwd returned: %s\n", buf);
    ASSERT(strcmp(buf, "/test_getcwd") == 0, "getcwd should match chdir");

    result = chdir("/");
    ASSERT(result == 0, "chdir() should succeed");

    result = rmdir("/test_getcwd");
    ASSERT(result == 0, "rmdir() should succeed");
}

TEST(fs_readdir_basic)
{
    printf("  Testing readdir...\n");

    int result = mkdir("/test_readdir", 0755);
    ASSERT(result == 0, "mkdir() should succeed");

    int fd1 = open("/test_readdir/file1", O_CREAT | O_RDWR, 0644);
    ASSERT(fd1 >= 0, "open() should succeed");

    result = close(fd1);
    ASSERT(result == 0, "close() should succeed");

    int fd2 = open("/test_readdir/file2", O_CREAT | O_RDWR, 0644);
    result = close(fd2);
    ASSERT(result == 0, "close() should succeed");

    int dirfd = open("/test_readdir", O_RDONLY, 0);
    ASSERT(dirfd >= 0, "Should open directory");

    dirent_t dirent;
    int count = 0;
    int ret;
    while ((ret = readdir(dirfd, &dirent, 1)) > 0) {
        printf("  Entry: %s (inode=%ld)\n", dirent.d_name, dirent.d_ino);
        count++;
    }

    ASSERT(ret == 0, "Dirent should return a 0");
    ASSERT(count >= 4, "Should have at least . .. file1 file2");
    printf("  Read %d directory entries\n", count);

    result = close(dirfd);
    ASSERT(result == 0, "close() should succeed");

    result = unlink("/test_readdir/file1");
    ASSERT(result == 0, "unlink() should succeed");

    result = unlink("/test_readdir/file2");
    ASSERT(result == 0, "unlink() should succeed");

    result = rmdir("/test_readdir");
    ASSERT(result == 0, "rmdir() should succeed");
}

TEST(fs_readdir_empty)
{
    printf("  Testing readdir on empty directory...\n");

    int result = mkdir("/test_empty", 0755);
    ASSERT(result == 0, "mkdir() should succeed");

    int dirfd = open("/test_empty", O_RDONLY, 0);
    ASSERT(dirfd >= 0, "Should open directory");

    dirent_t dirent;
    int count = 0;
    while (readdir(dirfd, &dirent, 1) > 0) {
        count++;
    }

    ASSERT(count == 2, "Empty directory should have . and ..");
    printf("  Empty directory has %d entries (. and ..)\n", count);

    result = close(dirfd);
    ASSERT(result == 0, "close() should succeed");

    result = rmdir("/test_empty");
    ASSERT(result == 0, "rmdir() should succeed");
}

void filesystem_tests(void)
{
    printf("\n========== FILESYSTEM TEST SUITE ==========\n\n");

    RUN_TEST(fs_mkdir_basic);
    RUN_TEST(fs_mkdir_trailing_slash);
    RUN_TEST(fs_mkdir_nested);
    RUN_TEST(fs_mkdir_already_exists);

    RUN_TEST(fs_rmdir_nonexistent);

    RUN_TEST(fs_open_create);
    RUN_TEST(fs_open_nonexistent);

    RUN_TEST(fs_unlink_basic);
    RUN_TEST(fs_unlink_nonexistent);

    // RUN_TEST(fs_chdir_basic);
    // RUN_TEST(fs_chdir_relative);
    // RUN_TEST(fs_chdir_dotdot);
    // RUN_TEST(fs_chdir_nonexistent);

    // RUN_TEST(fs_getcwd_basic);
    // RUN_TEST(fs_getcwd_after_chdir);

    RUN_TEST(fs_readdir_basic);
    RUN_TEST(fs_readdir_empty);

    printf("\n============================================\n");
}

int main(void)
{
    filesystem_tests();

    printf("\n========== TEST RESULTS ==========\n");
    printf("Passed: %u\n", tests_passed);
    printf("Failed: %u\n", tests_failed);

    if (tests_failed > 0) {
        printf("STATUS: FAILED ❌\n");
    } else {
        printf("STATUS: ALL TESTS PASSED ✅\n");
    }
    printf("====================================\n\n");

    return 0;
}
