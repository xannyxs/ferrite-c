#ifndef _RTC_H_
#define _RTC_H_

#include <stdint.h>

typedef struct {
  uint8_t second;
  uint8_t minute;
  uint8_t hour;
  uint8_t day_of_week;
  uint8_t day_of_month;
  uint8_t month;
  uint8_t year;
} time_t;

uint32_t getfrequency(void);

void settime(time_t *time);

void gettime(time_t *time);

void rtc_init(void);

#endif
