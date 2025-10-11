#ifndef TESTS_H
#define TESTS_H

#ifdef __TEST

#    include "drivers/printk.h"
#    include "drivers/video/vga.h"

#    include <stdbool.h>

#    define ASSERT(condition, message)                                     \
        do {                                                               \
            if (!(condition)) {                                            \
                printk("[FAIL] %s:%d: %s\n", __FILE__, __LINE__, message); \
                return false;                                              \
            }                                                              \
        } while (0)

#    define ASSERT_EQ(a, b, message) ASSERT((a) == (b), message)

#    define TEST(name)          \
        bool test_##name(void); \
        bool test_##name(void)

#    define RUN_TEST(name)                          \
        do {                                        \
            printk("[TEST] " #name "... ");         \
            if (test_##name()) {                    \
                vga_setcolor(VGA_COLOR_GREEN);      \
                printk("PASS");                     \
                vga_setcolor(VGA_COLOR_LIGHT_GREY); \
                printk("\n");                       \
                tests_passed++;                     \
            } else {                                \
                vga_setcolor(VGA_COLOR_RED);        \
                printk("FAIL");                     \
                vga_setcolor(VGA_COLOR_LIGHT_GREY); \
                printk("\n");                       \
                tests_failed++;                     \
            }                                       \
        } while (0)

void process_tests(void);

void idt_tests(void);

void main_tests(void);

void buddy_tests(void);

#endif /* __TEST */

#endif /* TESTS_H */
