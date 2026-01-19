#include "drivers/vga.h"
#include <drivers/printk.h>
#include <ferrite/string.h>
#include <lib/stdlib.h>

#define ASSERT(cond, msg) \
    do {                  \
        if (!(cond)) {    \
            abort(msg);   \
        }                 \
    } while (0)

void test_printk_formatting(void)
{
    char buf[256];

    snprintk(buf, sizeof(buf), "%d", 42);
    ASSERT(strcmp(buf, "42") == 0, "%d failed");

    snprintk(buf, sizeof(buf), "%i", 42);
    ASSERT(strcmp(buf, "42") == 0, "%i failed");

    snprintk(buf, sizeof(buf), "%c", 65);
    ASSERT(strcmp(buf, "A") == 0, "%c failed");

    snprintk(buf, sizeof(buf), "%x", 0xff);
    ASSERT(strcmp(buf, "ff") == 0, "%x failed");

    snprintk(buf, sizeof(buf), "%p", (void*)0xdeadbeef);
    ASSERT(strcmp(buf, "0xdeadbeef") == 0, "%p failed");

    snprintk(buf, sizeof(buf), "Hello %s", "world");
    ASSERT(strcmp(buf, "Hello world") == 0, "%s failed");

    snprintk(buf, sizeof(buf), "%u", 4294967295U);
    ASSERT(strcmp(buf, "4294967295") == 0, "%u failed");

    snprintk(buf, sizeof(buf), "%lu", 4294967295UL);
    ASSERT(strcmp(buf, "4294967295") == 0, "%lu failed");

    snprintk(buf, sizeof(buf), "%llu", 9223372036854775807ULL);
    ASSERT(strcmp(buf, "9223372036854775807") == 0, "%llu failed");

    snprintk(buf, sizeof(buf), "%08x", 0xff);
    ASSERT(strcmp(buf, "000000ff") == 0, "width padding failed");

    snprintk(buf, sizeof(buf), "%d", -42);
    ASSERT(strcmp(buf, "-42") == 0, "negative %d failed");

    snprintk(buf, sizeof(buf), "%d", 0);
    ASSERT(strcmp(buf, "0") == 0, "zero failed");

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-overflow"
    snprintk(buf, sizeof(buf), "%s", (char*)NULL);
#pragma GCC diagnostic pop

    ASSERT(strcmp(buf, "<NULL>") == 0, "NULL string failed");

    snprintk(buf, sizeof(buf), "%s", "");
    ASSERT(strcmp(buf, "") == 0, "empty string failed");

    snprintk(buf, sizeof(buf), "100%%");
    ASSERT(strcmp(buf, "100%") == 0, "%% failed");

    snprintk(buf, sizeof(buf), "%d %s %x", 42, "test", 0xff);
    ASSERT(strcmp(buf, "42 test ff") == 0, "multiple formats failed");

    snprintk(buf, sizeof(buf), "%8d", 42);
    ASSERT(strcmp(buf, "      42") == 0, "space padding failed");

    vga_setcolour(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    printk("All printk tests passed!\n");
    vga_setcolour(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}
