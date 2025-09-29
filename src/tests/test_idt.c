// tests/test_idt.c
#ifdef __TEST

#include "tests/tests.h"
#include <stdbool.h>

extern volatile uint64_t ticks;
extern uint32_t tests_passed;
extern uint32_t tests_failed;
static volatile int interrupt_handled = 0;

TEST(breakpoint_instruction) {
  printk("  Testing breakpoint (int 3)...\n");
  __asm__ volatile("int $0x03");
  printk("  Returned from breakpoint\n");
  return true;
}

TEST(timer_interrupt_fires) {
  uint32_t start_ticks = ticks;

  for (volatile int i = 0; i < 10000000; i++)
    ;

  uint32_t end_ticks = ticks;
  ASSERT(end_ticks > start_ticks, "Timer should have incremented");
  printk("  Timer ticks: %u -> %u\n", start_ticks, end_ticks);
  return true;
}

void idt_tests(void) {
  printk("\n========== IDT TEST SUITE ==========\n\n");
  RUN_TEST(breakpoint_instruction);
  RUN_TEST(timer_interrupt_fires);
}

#endif
