#ifndef MATH_H
#define MATH_H

#include <stdbool.h>
#include <stdint.h>

#define likely(x) __builtin_expect(!!(x), true)
#define unlikely(x) __builtin_expect(!!(x), false)

#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1))
#define CEIL_DIV(a, b) (((a + b - 1) / b))

static inline uint32_t log2(uint32_t n) {
  if (unlikely(n == 0)) {
    __builtin_trap();
  }

  return (sizeof(uint32_t) * 8 - 1) - __builtin_clz(n);
}

#endif /* MATH_H */
