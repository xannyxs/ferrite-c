#include "rtc.h"
#include "arch/x86/io.h"
#include "arch/x86/time/time.h"
#include "debug/debug.h"
#include "drivers/printk.h"
#include "lib/stdlib.h"

#include <stdbool.h>
#include <stdint.h>

#define REG_A 0x8A
#define REG_B 0x8B

uint8_t rate = 14;
uint32_t frequency = 0;
rtc_time_t global_time;

static inline uint8_t read_rtc_register(int32_t reg) {
  outb(0x70, reg);
  return inb(0x71);
}

static inline uint8_t bcd_to_bin(uint8_t bcd) {
  return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

inline uint32_t getfrequency(void) { return frequency; }

void rtc_read_time(void) {
  while (read_rtc_register(0x0A) & 0x80)
    ;

  global_time.second = read_rtc_register(0x00);
  global_time.minute = read_rtc_register(0x02);
  global_time.hour = read_rtc_register(0x04);
  global_time.month = read_rtc_register(0x08);
  global_time.year = read_rtc_register(0x09);
  global_time.day = read_rtc_register(0x07);

  if (!(read_rtc_register(0x0B) & 0x04)) {
    global_time.second = bcd_to_bin(global_time.second);
    global_time.minute = bcd_to_bin(global_time.minute);
    global_time.hour = bcd_to_bin(global_time.hour);
    global_time.month = bcd_to_bin(global_time.month);
    global_time.year = bcd_to_bin(global_time.year);
    global_time.day = bcd_to_bin(global_time.day);
  }
}

void rtc_init(void) {
  frequency = 32768 >> (rate - 1);

  outb(0x70, REG_A);                  // Select Register A, disable NMI
  char prev_a = inb(0x71);            // Read current value of Register A
  outb(0x70, REG_A);                  // Re-select Register A
  outb(0x71, (prev_a & 0xF0) | rate); // Write new rate to Register A

  outb(0x70, REG_B);         // Select Register B, disable NMI
  char prev_b = inb(0x71);   // Read current value of Register B
  outb(0x70, REG_B);         // Re-select Register B
  outb(0x71, prev_b | 0x40); // Enable bit 6 (Periodic Interrupt)

  rtc_read_time();

  printk("RTC Time: 20%d-%d-%d %d:%d:%d\n", global_time.year, global_time.month,
         global_time.day, global_time.hour, global_time.minute,
         global_time.second);

  time_t epoch = to_epoch(&global_time);
  setepoch(epoch);
}
