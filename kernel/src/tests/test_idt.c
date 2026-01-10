#ifdef __TEST

#    include "tests/tests.h"
#    include <ferrite/types.h>

#    include <stdbool.h>

extern unsigned long long volatile ticks;
extern u32 tests_passed;
extern u32 tests_failed;
static int volatile interrupt_handled = 0;

TEST(breakpoint_instruction)
{
    printk("  Testing breakpoint (int 3)...\n");
    __asm__ volatile("int $0x03");
    printk("  Returned from breakpoint\n");
    do_exit(0);
}

TEST(timer_interrupt_fires)
{
    u32 start_ticks = ticks;

    for (int volatile i = 0; i < 10000000; i++)
        ;

    u32 end_ticks = ticks;
    ASSERT(end_ticks > start_ticks, "Timer should have incremented");
    printk("  Timer ticks: %u -> %u\n", start_ticks, end_ticks);
    do_exit(0);
}

void idt_tests(void)
{
    printk("\n========== IDT TEST SUITE ==========\n\n");
    RUN_TEST(breakpoint_instruction);
    RUN_TEST(timer_interrupt_fires);
}

#endif

typedef int _test_translation_unit_not_empty;
