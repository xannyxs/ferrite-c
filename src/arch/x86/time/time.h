#ifndef TIME_H
#define TIME_H

#include "rtc.h"

#include <stdbool.h>
#include <stdint.h>

typedef int64_t time_t;

void gettime(rtc_time_t* t);

time_t getepoch(void);

void setepoch(time_t new);

time_t to_epoch(rtc_time_t* t);

void from_epoch(time_t epoch, rtc_time_t* t);

#endif /* TIME_H */
