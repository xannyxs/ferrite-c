#include "time.h"
#include "rtc.h"

#include <stdbool.h>
#include <stdint.h>

volatile time_t seconds_since_epoch = 0;

inline void gettime(rtc_time_t *t) { from_epoch(seconds_since_epoch, t); }

__attribute__((warn_unused_result)) inline time_t getepoch(void) {
  return seconds_since_epoch;
}

inline void setepoch(time_t new) { seconds_since_epoch = new; }

static inline int32_t is_leap(uint32_t year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

time_t to_epoch(rtc_time_t *t) {
  uint64_t epoch_seconds;
  uint32_t full_year = 2000 + t->year;
  uint32_t days_since_epoch = 0;
  const uint32_t days_in_month[] = {0,  31, 28, 31, 30, 31, 30,
                                    31, 31, 30, 31, 30, 31};

  for (uint32_t y = 1970; y < full_year; ++y) {
    days_since_epoch += is_leap(y) ? 366 : 365;
  }

  for (uint8_t m = 1; m < t->month; ++m) {
    days_since_epoch += days_in_month[m];
    if (m == 2 && is_leap(full_year)) {
      days_since_epoch++;
    }
  }

  days_since_epoch += t->day - 1;

  epoch_seconds = days_since_epoch * 86400ULL + t->hour * 3600ULL +
                  t->minute * 60ULL + t->second;

  return epoch_seconds;
}

void from_epoch(time_t epoch, rtc_time_t *t) {
  const uint32_t days_in_month[] = {0,  31, 28, 31, 30, 31, 30,
                                    31, 31, 30, 31, 30, 31};
  uint32_t current_year = 1970;
  uint32_t days_in_current_year;

  uint64_t seconds_of_day = epoch % 86400ULL;
  t->hour = seconds_of_day / 3600;
  t->minute = (seconds_of_day % 3600) / 60;
  t->second = seconds_of_day % 60;

  uint64_t total_days = epoch / 86400ULL;

  while (true) {
    days_in_current_year = is_leap(current_year) ? 366 : 365;
    if (total_days >= days_in_current_year) {
      total_days -= days_in_current_year;
      current_year++;
    } else {
      break;
    }
  }

  t->year = (current_year >= 2000) ? (current_year - 2000) : 0;

  for (t->month = 1; t->month <= 12; ++t->month) {
    unsigned int days_this_month = days_in_month[t->month];
    if (t->month == 2 && is_leap(current_year)) {
      days_this_month++;
    }
    if (total_days >= days_this_month) {
      total_days -= days_this_month;
    } else {
      break;
    }
  }

  t->day = total_days + 1;
}
