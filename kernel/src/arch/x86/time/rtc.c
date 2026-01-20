#include "rtc.h"
#include "arch/x86/io.h"
#include "arch/x86/time/time.h"
#include "drivers/printk.h"
#include <types.h>

#include <stdbool.h>

#define RTC_REG_A 0x0A
#define RTC_REG_B 0x0B
#define BCD_TO_BIN(n) ((n >> 4) * 10) + (n & 0x0F);

static inline u8 read_rtc_register(s32 reg)
{
    outb(0x70, reg);
    return inb(0x71);
}

static void rtc_read_time(rtc_time_t* time)
{
    while (read_rtc_register(RTC_REG_A) & 0x80)
        ;

    time->second = read_rtc_register(0x00);
    time->minute = read_rtc_register(0x02);
    time->hour = read_rtc_register(0x04);
    time->month = read_rtc_register(0x08);
    time->year = read_rtc_register(0x09);
    time->day = read_rtc_register(0x07);

    if (!(read_rtc_register(RTC_REG_B) & 0x04)) {
        time->second = BCD_TO_BIN(time->second);
        time->minute = BCD_TO_BIN(time->minute);
        time->hour = BCD_TO_BIN(time->hour);
        time->month = BCD_TO_BIN(time->month);
        time->year = BCD_TO_BIN(time->year);
        time->day = BCD_TO_BIN(time->day);
    }
}

void rtc_init(void)
{
    rtc_time_t time;

    outb(0x70, 0x8B);           // Select Register B, disable NMI
    u8 prev_b = inb(0x71);      // Read current value of Register B
    outb(0x70, 0x8B);           // Re-select Register B
    outb(0x71, prev_b & ~0x40); // Write back with bit 6 (PIE) cleared

    rtc_read_time(&time);
    printk(
        "RTC Time: 20%d-%d-%d %d:%d:%d\n", time.year, time.month, time.day,
        time.hour, time.minute, time.second
    );

    time_t epoch = to_epoch(&time);
    setepoch(epoch);
}
