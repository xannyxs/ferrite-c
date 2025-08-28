#ifndef _RTC_H_
#define _RTC_H_

#include <stdint.h>

typedef struct {
  uint8_t second;
  uint8_t minute;
  uint8_t hour;
  uint8_t day;
  uint8_t month;
  uint8_t year;
} rtc_time_t;

void rtc_init(void);

#endif
