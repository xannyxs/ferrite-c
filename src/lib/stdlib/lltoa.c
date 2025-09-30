#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static void simple_strcpy(char* dest, char const* src)
{
    while (*src)
        *dest++ = *src++;
    *dest = '\0';
}

char* lltoa_buf(int64_t n, char* buffer, size_t buffer_size)
{
    size_t const MIN_BUF_SIZE_FOR_INT64_MIN = 21;

    if (n == INT64_MIN) {
        if (buffer_size < MIN_BUF_SIZE_FOR_INT64_MIN)
            return (NULL);
        simple_strcpy(buffer, "-9223372036854775808");
        return (buffer);
    }

    bool is_negative = (n < 0);
    uint64_t num;
    size_t len = 0;

    if (is_negative) {
        num = -n;
        len = 1; // For the '-' sign
    } else {
        num = n;
    }

    uint64_t temp = num;
    if (temp == 0) {
        len = 1;
    } else {
        while (temp > 0) {
            temp /= 10;
            len++;
        }
    }

    if (buffer_size < len + 1)
        return (NULL);

    buffer[len] = '\0';
    do {
        buffer[--len] = (num % 10) + '0';
        num /= 10;
    } while (num > 0);

    if (is_negative)
        buffer[0] = '-';

    return (buffer);
}
