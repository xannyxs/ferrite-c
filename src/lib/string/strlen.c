#include <stddef.h>

__attribute__((target("general-regs-only"))) size_t strlen(const char *s) {
  size_t len = 0;

  while (s[len]) {
    len++;
  }

  return len;
}
