#include "time.h"
#include "rtc.h"
#include <types.h>

#include <stdbool.h>

time_t volatile seconds_since_epoch = 0;

inline void gettime(rtc_time_t* t) { from_epoch(seconds_since_epoch, t); }

__attribute__((warn_unused_result)) inline time_t getepoch(void)
{
    return seconds_since_epoch;
}

inline void setepoch(time_t const new) { seconds_since_epoch = new; }

static inline s32 is_leap(u32 year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

time_t to_epoch(rtc_time_t* t)
{
    u32 full_year = 2000 + t->year;
    u32 days_since_epoch = 0;

    for (u32 y = 1970; y < full_year; ++y) {
        days_since_epoch += is_leap(y) ? 366 : 365;
    }

    for (u8 m = 1; m < t->month; ++m) {
        u32 const days_in_month[]
            = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
        days_since_epoch += days_in_month[m];
        if (m == 2 && is_leap(full_year)) {
            days_since_epoch++;
        }
    }

    days_since_epoch += t->day - 1;

    time_t epoch_seconds = (days_since_epoch * 86400ULL)
        + (t->hour * 3600ULL) + (t->minute * 60ULL) + t->second;

    return epoch_seconds;
}

void from_epoch(time_t epoch, rtc_time_t* t)
{
    u32 current_year = 1970;

    unsigned long long const seconds_of_day = epoch % 86400ULL;
    t->hour = seconds_of_day / 3600;
    t->minute = (seconds_of_day % 3600) / 60;
    t->second = seconds_of_day % 60;

    unsigned long long total_days = epoch / 86400ULL;

    while (true) {
        u32 days_in_current_year = is_leap(current_year) ? 366 : 365;
        if (total_days >= days_in_current_year) {
            total_days -= days_in_current_year;
            current_year++;
        } else {
            break;
        }
    }

    t->year = (current_year >= 2000) ? (current_year - 2000) : 0;

    for (t->month = 1; t->month <= 12; ++t->month) {
        u32 const days_in_month[]
            = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
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
