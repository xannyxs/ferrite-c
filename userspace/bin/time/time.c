#include <libc/stdio.h>
#include <libc/syscalls.h>
#include <uapi/types.h>

#define EPOCH_YEAR 1970
#define SECS_PER_MIN 60
#define SECS_PER_HOUR 3600
#define SECS_PER_DAY 86400

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

static int is_leap_year(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static int days_in_month(int month, int year)
{
    static int const days[]
        = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (month == 1 && is_leap_year(year)) {
        return 29;
    }
    return days[month];
}

struct tm* localtime(time_t const* timer)
{
    static struct tm result;
    time_t t = *timer;

    result.tm_sec = t % 60;
    t /= 60;
    result.tm_min = t % 60;
    t /= 60;
    result.tm_hour = t % 24;
    t /= 24;

    result.tm_wday = (t + 4) % 7;

    int year = EPOCH_YEAR;
    int days = t;

    while (1) {
        int days_this_year = is_leap_year(year) ? 366 : 365;
        if (days < days_this_year) {
            break;
        }
        days -= days_this_year;
        year++;
    }

    result.tm_year = year - 1900;
    result.tm_yday = days;

    int month = 0;
    while (days >= days_in_month(month, year)) {
        days -= days_in_month(month, year);
        month++;
    }

    result.tm_mon = month;
    result.tm_mday = days + 1;
    result.tm_isdst = -1;

    return &result;
}

int main(void)
{
    printf("UNIX TIME: %d\n", time(NULL));

    time_t const t = time(NULL);
    struct tm* tm = localtime(&t);
    printf(
        "%04d-%02d-%02d %02d:%02d:%02d\n", tm->tm_year + 1900, tm->tm_mon + 1,
        tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec
    );

    return 0;
}
