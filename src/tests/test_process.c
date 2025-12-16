#ifdef __TEST
#    include "sys/process/process.h"
#    include "sys/timer/timer.h"
#    include "tests/tests.h"
#    include <ferrite/types.h>

#    include <stdbool.h>

extern u32 tests_passed;
extern u32 tests_failed;

// ========== Test Helper Processes ==========

void test_child(void)
{
    printk("  [child %d] Started and exiting\n", myproc()->pid);
    do_exit(0);
}

void test_child_exit_42(void)
{
    printk("  [child %d] Exiting with status 42\n", myproc()->pid);
    do_exit(42);
}

void test_child_exit_123(void)
{
    printk("  [child %d] Exiting with status 123\n", myproc()->pid);
    do_exit(123);
}

void test_child_with_work(void)
{
    printk("  [child %d] Doing work before exit\n", myproc()->pid);
    for (s32 volatile i = 0; i < 1000000; i++) {
        // Busy work
    }
    printk("  [child %d] Work done, exiting\n", myproc()->pid);
    do_exit(0);
}

void test_child_sleeps(void)
{
    printk("  [child %d] Sleeping for 2 seconds\n", myproc()->pid);
    ksleep(2);
    printk("  [child %d] Woke up, exiting\n", myproc()->pid);
    do_exit(99);
}

// ========== Test Cases ==========

TEST(parent_child_wait_exit)
{
    pid_t child = do_exec("child", test_child);

    // proc_t *p = getpid();
    // p->parent = NULL;

    ASSERT(child > 0, "Failed to create child");

    pid_t result = do_wait(NULL);
    ASSERT_EQ(result, child, "Parent should reap child");
    do_exit(0);
}

TEST(wait_with_no_children)
{
    s32 result = do_wait(NULL);
    ASSERT_EQ(result, -1, "wait() should return -1 with no children");
    do_exit(0);
}

TEST(wait_returns_exit_status)
{
    pid_t child = do_exec("child", test_child_exit_42);
    ASSERT(child > 0, "Failed to create child");

    s32 status;
    pid_t result = do_wait(&status);

    ASSERT_EQ(result, child, "Should return child PID");
    ASSERT_EQ(status, 42, "Exit status should be 42");
    do_exit(0);
}

TEST(wait_returns_different_exit_status)
{
    pid_t child = do_exec("child", test_child_exit_123);
    ASSERT(child > 0, "Failed to create child");

    s32 status;
    pid_t result = do_wait(&status);

    ASSERT_EQ(result, child, "Should return child PID");
    ASSERT_EQ(status, 123, "Exit status should be 123");
    do_exit(0);
}

TEST(wait_with_multiple_children)
{
    pid_t child1 = do_exec("child1", test_child);
    pid_t child2 = do_exec("child2", test_child);

    ASSERT(child1 > 0, "Failed to create child1");
    ASSERT(child2 > 0, "Failed to create child2");

    pid_t result1 = do_wait(NULL);
    pid_t result2 = do_wait(NULL);

    // Should reap both children (order doesn't matter)
    ASSERT(
        (result1 == child1 || result1 == child2),
        "First wait should return a valid child"
    );
    ASSERT(
        (result2 == child1 || result2 == child2),
        "Second wait should return a valid child"
    );
    ASSERT(result1 != result2, "Should reap different children");

    do_exit(0);
}

TEST(wait_after_all_children_exit)
{
    pid_t child = do_exec("child", test_child);
    ASSERT(child > 0, "Failed to create child");

    // Wait for child
    pid_t result1 = do_wait(NULL);
    ASSERT_EQ(result1, child, "Should reap child");

    // Try to wait again - should return -1
    pid_t result2 = do_wait(NULL);
    ASSERT_EQ(result2, -1, "Second wait should return -1 (no more children)");

    do_exit(0);
}

TEST(wait_for_child_that_does_work)
{
    pid_t child = do_exec("worker_child", test_child_with_work);
    ASSERT(child > 0, "Failed to create worker child");

    s32 status;
    pid_t result = do_wait(&status);

    ASSERT_EQ(result, child, "Should reap worker child");
    ASSERT_EQ(status, 0, "Worker should exit with status 0");
    do_exit(0);
}

TEST(wait_for_sleeping_child)
{
    printk("  (This test will take 2 seconds...)\n");
    pid_t child = do_exec("sleeping_child", test_child_sleeps);
    ASSERT(child > 0, "Failed to create sleeping child");

    s32 status;
    pid_t result = do_wait(&status);

    ASSERT_EQ(result, child, "Should reap sleeping child");
    ASSERT_EQ(status, 99, "Sleeping child should exit with status 99");
    do_exit(0);
}

TEST(wait_with_three_children)
{
    pid_t child1 = do_exec("child1", test_child);
    pid_t child2 = do_exec("child2", test_child);
    pid_t child3 = do_exec("child3", test_child);

    ASSERT(child1 > 0 && child2 > 0 && child3 > 0, "Failed to create children");

    // Reap all three
    pid_t r1 = do_wait(NULL);
    pid_t r2 = do_wait(NULL);
    pid_t r3 = do_wait(NULL);

    // Check all were reaped
    ASSERT(r1 > 0 && r2 > 0 && r3 > 0, "All waits should return valid PIDs");
    ASSERT(r1 != r2 && r2 != r3 && r1 != r3, "All PIDs should be different");

    // Fourth wait should fail
    pid_t r4 = do_wait(NULL);
    ASSERT_EQ(r4, -1, "Fourth wait should return -1");

    do_exit(0);
}

TEST(wait_preserves_null_status)
{
    pid_t child = do_exec("child", test_child_exit_42);
    ASSERT(child > 0, "Failed to create child");

    // Passing NULL should not crash
    pid_t result = do_wait(NULL);
    ASSERT_EQ(result, child, "Should reap child even with NULL status");

    do_exit(0);
}

// ========== Test Runner ==========

void process_tests(void)
{
    printk("\n========== WAIT/EXIT TEST SUITE ==========\n\n");

    RUN_TEST(wait_with_no_children);
    RUN_TEST(parent_child_wait_exit);
    RUN_TEST(wait_returns_exit_status);
    RUN_TEST(wait_returns_different_exit_status);
    RUN_TEST(wait_preserves_null_status);
    RUN_TEST(wait_after_all_children_exit);
    RUN_TEST(wait_with_multiple_children);
    RUN_TEST(wait_with_three_children);
    RUN_TEST(wait_for_child_that_does_work);
    RUN_TEST(wait_for_sleeping_child);
}

#endif /* __TEST */

typedef int _test_translation_unit_not_empty;
