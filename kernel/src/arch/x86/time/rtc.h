#ifndef RTC_H
#define RTC_H

#include <ferrite/types.h>

typedef struct {
    u8 second;
    u8 minute;
    u8 hour;
    u8 day;
    u8 month;
    u8 year;
} rtc_time_t;

void rtc_init(void);

#endif /* RTC_H */
