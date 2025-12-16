#ifdef __TEST

#    include "arch/x86/io.h"
#    include "tests.h"
#    include <ferrite/types.h>

u32 tests_passed = 0;
u32 tests_failed = 0;

void main_tests(void)
{
    privilege_tests();
    idt_tests();
    buddy_tests();
    process_tests();
    filesystem_tests();
    socket_tests();

    printk("\n========== TEST RESULTS ==========\n");
    printk("Passed: %u\n", tests_passed);
    printk("Failed: %u\n", tests_failed);

    if (tests_failed > 0) {
        printk("STATUS: FAILED ❌\n");
    } else {
        printk("STATUS: ALL TESTS PASSED ✅\n");
    }
    printk("====================================\n\n");

    qemu_exit(0);
}

#endif /* __TEST */

typedef int _test_translation_unit_not_empty;
