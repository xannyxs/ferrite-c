#ifndef TESTS_H
#define TESTS_H

#ifdef __TEST

#    include "drivers/printk.h"
#    include "drivers/vga.h"
#    include "sys/process/process.h"

#    include <stdbool.h>

#    define ASSERT(cond, msg)                                      \
        if (!(cond)) {                                             \
            printk("[FAIL] %s:%d: %s\n", __FILE__, __LINE__, msg); \
            do_exit(1);                                            \
        }

#    define ASSERT_EQ(a, b, message) ASSERT((a) == (b), message)

#    define TEST(name) static void __attribute__((unused)) test_##name(void)

#    define RUN_TEST(name)                                            \
        do {                                                          \
            printk("[TEST] " #name "... ");                           \
            do_exec(#name, test_##name);                              \
            int status;                                               \
            do_wait(&status);                                         \
                                                                      \
            if (status == 0) {                                        \
                vga_setcolour(VGA_COLOR_GREEN, VGA_COLOR_BLACK);      \
                printk("PASS\n");                                     \
                vga_setcolour(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK); \
                tests_passed++;                                       \
            } else {                                                  \
                vga_setcolour(VGA_COLOR_RED, VGA_COLOR_BLACK);        \
                printk("FAIL\n");                                     \
                vga_setcolour(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK); \
                tests_failed++;                                       \
            }                                                         \
        } while (0)

void process_tests(void);

void idt_tests(void);

void main_tests(void);

void buddy_tests(void);

void socket_tests(void);

void filesystem_tests(void);

void privilege_tests(void);

#endif /* __TEST */

#endif /* TESTS_H */
