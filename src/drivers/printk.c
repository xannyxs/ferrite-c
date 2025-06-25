#include "video/vga.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

char buf[1024];

/* Private */

static char *simple_number(char *str, long num, int32_t base, bool is_signed) {
  char tmp[36];
  int i = 0;

  if (is_signed && base == 10 && num < 0) {
    *str++ = '-';
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

  while (i-- > 0) {
    *str++ = tmp[i];
  }

  return str;
}

static int kfmt(char *buf, const char *fmt, va_list args) {
  char *str = buf;

  for (; *fmt; ++fmt) {
    if (*fmt != '%') {
      *str++ = *fmt;
      continue;
    }

    ++fmt;

    switch (*fmt) {
    case 'c': {
      *str++ = (unsigned char)va_arg(args, int);
      break;
    }
    case 's': {
      const char *s = va_arg(args, const char *);
      if (!s) {
        s = "<NULL>";
      }

      size_t len = strlen(s);

      for (size_t i = 0; i < len; ++i) {
        *str++ = *s++;
      }
      break;
    }
    case 'd':
    case 'i': {
      long i = va_arg(args, long);
      str = simple_number(str, i, 10, 1);
      break;
    }
    case 'u': {
      unsigned long u = va_arg(args, unsigned long);
      str = simple_number(str, u, 10, 0);
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

int32_t printk(const char *fmt, ...) {
  va_list args;

  va_start(args, fmt);
  int32_t len = kfmt(buf, fmt, args);
  va_end(args);

  for (int i = 0; i < len; i++) {
    vga_putchar(buf[i]);
  }

  return len;
}
