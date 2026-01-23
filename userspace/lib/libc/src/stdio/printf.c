#include <stdarg.h>
#include <stdbool.h>

#include <libc/string.h>
#include <libc/syscalls.h>

static char buf[1024];

/* Private */

static char* simple_number_64(
    char* str,
    unsigned long long num,
    int base,
    bool is_signed,
    int width,
    bool zero_pad
)
{
    char tmp[65];
    int i = 0;
    bool negative = false;

    if (is_signed && base == 10 && (long long)num < 0) {
        negative = true;
        num = -(long long)num;
    }

    if (num == 0) {
        tmp[i++] = '0';
    } else {
        while (num != 0) {
            unsigned long long rem = num % base;
            tmp[i++] = (char)((rem > 9) ? (rem - 10) + 'a' : rem + '0');
            num /= base;
        }
    }

    int num_len = i + (negative ? 1 : 0);
    if (width > num_len) {
        int pad_count = width - num_len;
        if (zero_pad && negative) {
            *str++ = '-';
            negative = false;
        }
        char pad_char = zero_pad ? '0' : ' ';
        while (pad_count-- > 0) {
            *str++ = pad_char;
        }
    }

    if (negative) {
        *str++ = '-';
    }

    while (i-- > 0) {
        *str++ = tmp[i];
    }

    return str;
}

static char* simple_number(
    char* str,
    long num,
    int base,
    bool is_signed,
    int width,
    bool zero_pad
)
{
    char tmp[36];
    int i = 0;
    bool negative = false;

    if (is_signed && base == 10 && num < 0) {
        negative = true;
        num = -num;
    }

    unsigned int unum = (unsigned int)num;
    if (unum == 0) {
        tmp[i++] = '0';
    } else {
        while (unum != 0) {
            unsigned int rem = unum % base;
            tmp[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
            unum /= base;
        }
    }

    int num_len = i + (negative ? 1 : 0);
    if (width > num_len) {
        int pad_count = width - num_len;
        if (zero_pad && negative) {
            *str++ = '-';
            negative = false;
        }
        char pad_char = zero_pad ? '0' : ' ';
        while (pad_count-- > 0) {
            *str++ = pad_char;
        }
    }

    if (negative) {
        *str++ = '-';
    }

    while (i-- > 0) {
        *str++ = tmp[i];
    }

    return str;
}

static int kfmt(char* buf, char const* fmt, va_list args)
{
    char* str = buf;

    for (; *fmt; ++fmt) {
        if (*fmt != '%') {
            *str++ = *fmt;
            continue;
        }
        ++fmt;

        bool zero_pad = false;
        if (*fmt == '0') {
            zero_pad = true;
            ++fmt;
        }

        int width = 0;
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            ++fmt;
        }

        int precision = -1;
        if (*fmt == '.') {
            ++fmt;
            precision = 0;
            while (*fmt >= '0' && *fmt <= '9') {
                precision = precision * 10 + (*fmt - '0');
                ++fmt;
            }
        }

        int length = 0;
        if (*fmt == 'l') {
            length = 1;
            ++fmt;
            if (*fmt == 'l') {
                length = 2;
                ++fmt;
            }
        }

        switch (*fmt) {
        case 'c': {
            *str++ = (unsigned char)va_arg(args, int);
            break;
        }
        case 's': {
            char* s = va_arg(args, char*);
            if (!s) {
                s = "<NULL>";
            }

            size_t len = strlen(s);
            if (precision >= 0 && (size_t)precision < len) {
                len = precision;
            }
            if (width > (int)len) {
                int pad = width - len;
                while (pad > 0) {
                    *str++ = ' ';
                    pad -= 1;
                }
            }

            for (size_t i = 0; i < len; ++i) {
                *str++ = *s++;
            }

            break;
        }

        case 'x':
        case 'X': {
            str = simple_number(
                str, va_arg(args, unsigned int), 16, 0, width, zero_pad
            );
            break;
        }

        case 'p': {
            *str++ = '0';
            *str++ = 'x';
            str = simple_number(str, va_arg(args, unsigned int), 16, 0, 8, 1);
            break;
        }

        case 'd':
        case 'i': {
            int i = va_arg(args, int);
            str = simple_number(str, i, 10, 1, width, zero_pad);
            break;
        }

        case 'u': {
            if (length == 2) {
                unsigned long long ull = va_arg(args, unsigned long long);
                str = simple_number_64(str, ull, 10, 0, width, zero_pad);
            } else if (length == 1) {
                unsigned long ul = va_arg(args, unsigned long);
                str = simple_number(str, ul, 10, 0, width, zero_pad);
            } else {
                unsigned int u = va_arg(args, unsigned int);
                str = simple_number(str, u, 10, 0, width, zero_pad);
            }
            break;
        }

        case '%':
            *str++ = '%';
            break;

        default: {
            *str++ = '%';
            if (*fmt) {
                *str++ = *fmt;
            } else {
                --fmt;
            }
            break;
        }
        }
    }

    *str = '\0';
    return str - buf;
}

/* Public */

int printf(char const* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int len = kfmt(buf, fmt, args);
    va_end(args);

    write(1, buf, len);

    return len;
}

int snprintf(char* local_buf, size_t size, char const* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int len = kfmt(local_buf, fmt, args);

    va_end(args);

    if (len >= (int)size) {
        local_buf[size - 1] = '\0';
        return size - 1;
    }

    return len;
}
