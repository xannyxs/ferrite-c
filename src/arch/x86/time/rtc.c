#include "rtc.h"
#include "arch/x86/idt/idt.h"
#include "arch/x86/io.h"
#include "arch/x86/time/time.h"
#include "drivers/printk.h"

#include <stdbool.h>
#include <stdint.h>

#define REG_A 0x8A
#define REG_B 0x8B

uint8_t rate = 14;
uint32_t frequency = 0;
volatile uint64_t ticks = 0;

static inline uint8_t read_rtc_register(int32_t reg) {
  outb(0x70, reg);
  return inb(0x71);
}

static inline uint8_t bcd_to_bin(uint8_t bcd) {
  return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

inline uint32_t getfrequency(void) { return frequency; }

static void rtc_read_time(rtc_time_t *time) {
  while (read_rtc_register(0x0A) & 0x80)
    ;

  time->second = read_rtc_register(0x00);
  time->minute = read_rtc_register(0x02);
  time->hour = read_rtc_register(0x04);
  time->month = read_rtc_register(0x08);
  time->year = read_rtc_register(0x09);
  time->day = read_rtc_register(0x07);

  if (!(read_rtc_register(0x0B) & 0x04)) {
    time->second = bcd_to_bin(time->second);
    time->minute = bcd_to_bin(time->minute);
    time->hour = bcd_to_bin(time->hour);
    time->month = bcd_to_bin(time->month);
    time->year = bcd_to_bin(time->year);
    time->day = bcd_to_bin(time->day);
  }
}

void rtc_init(void) {
  rtc_time_t time;
  frequency = 32768 >> (rate - 1);

  outb(0x70, REG_A);                  // Select Register A, disable NMI
  char prev_a = inb(0x71);            // Read current value of Register A
  outb(0x70, REG_A);                  // Re-select Register A
  outb(0x71, (prev_a & 0xF0) | rate); // Write new rate to Register A

  outb(0x70, REG_B);         // Select Register B, disable NMI
  char prev_b = inb(0x71);   // Read current value of Register B
  outb(0x70, REG_B);         // Re-select Register B
  outb(0x71, prev_b | 0x40); // Enable bit 6 (Periodic Interrupt)

  rtc_read_time(&time);

  printk("RTC Time: 20%d-%d-%d %d:%d:%d\n", time.year, time.month, time.day,
         time.hour, time.minute, time.second);

  time_t epoch = to_epoch(&time);
  setepoch(epoch);
}

void rtc_task(struct interrupt_frame *frame) {
  (void)frame;

  outb(0x70, 0x0C); // select register C
  inb(0x71);        // just throw away contents

  ticks += 1;
  int32_t frequency = getfrequency();
  uint64_t seconds_since_epoch = getepoch();

  if (ticks % frequency == 0) {
    seconds_since_epoch += 1;

    setepoch(seconds_since_epoch);
  }
}
