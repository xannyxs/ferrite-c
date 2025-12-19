#include "debug/debug.h"
#include "ferrite/string.h"
#include "video/vga.h"

#include <stdarg.h>
#include <stdbool.h>

#if defined(__print_serial)
#    include "drivers/serial.h"
#endif

char buf[1024];

/* Private */

__attribute__((target("general-regs-only"))) static char* simple_number(
    char* str,
    long num,
    s32 base,
    bool is_signed,
    s32 width,
    bool zero_pad
)
{
    char tmp[36];
    s32 i = 0;
    bool negative = false;

    if (is_signed && base == 10 && num < 0) {
        negative = true;
        num = -num;
    }

    if (num == 0) {
        tmp[i++] = '0';
    } else {
        while (num != 0) {
            unsigned long rem = (unsigned long)num % base;
            tmp[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
            num = (unsigned long)num / base;
        }
    }

    s32 num_len = i + (negative ? 1 : 0);
    if (width > num_len) {
        s32 pad_count = width - num_len;
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

__attribute__((target("general-regs-only"))) static s32
kfmt(char* buf, char const* fmt, va_list args)
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

        s32 width = 0;
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            ++fmt;
        }

        s32 precision = -1;
        if (*fmt == '.') {
            ++fmt;
            precision = 0;
            while (*fmt >= '0' && *fmt <= '9') {
                precision = precision * 10 + (*fmt - '0');
                ++fmt;
            }
        }

        switch (*fmt) {
        case 'c': {
            *str++ = (unsigned char)va_arg(args, int);
            break;
        }
        case 's': {
            char const* s = va_arg(args, char const*);
            if (!s) {
                s = "<NULL>";
            }
            size_t len = strlen(s);
            if (precision >= 0 && (size_t)precision < len) {
                len = precision;
            }
            if (width > (s32)len) {
                s32 pad = width - len;
                while (pad-- > 0) {
                    *str++ = ' ';
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
                str, va_arg(args, unsigned long), 16, 0, width, zero_pad
            );
            break;
        }
        case 'd':
        case 'i': {
            long i = va_arg(args, long);
            str = simple_number(str, i, 10, 1, width, zero_pad);
            break;
        }
        case 'u': {
            unsigned long const u = va_arg(args, unsigned long);
            str = simple_number(str, u, 10, 0, width, zero_pad);
            break;
        }
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

__attribute__((target("general-regs-only"))) s32 printk(char const* fmt, ...)
{
    cli();

    va_list args;
    va_start(args, fmt);
    s32 len = kfmt(buf, fmt, args);
    va_end(args);
    vga_writestring(buf);

#if defined(__print_serial)
    serial_write_string(buf);
#endif
#if defined(__bochs)
    bochs_print_string(buf);
#endif

    sti();
    return len;
}
