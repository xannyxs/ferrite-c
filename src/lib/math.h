#ifndef MATH_H
#define MATH_H

#include <stdbool.h>
#include <stdint.h>

#define likely(x) __builtin_expect(!!(x), true)
#define unlikely(x) __builtin_expect(!!(x), false)

#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1))
#define CEIL_DIV(a, b) (((a + b - 1) / b))

/**
 * @brief  Calculates the base-2 logarithm of n, rounded down (floor).
 */
static inline uint32_t floor_log2(uint32_t n)
{
    if (unlikely(n == 0)) {
        __builtin_trap();
    }

    return (sizeof(uint32_t) * 8 - 1) - __builtin_clz(n);
}

/**
 * @brief  Calculates the base-2 logarithm of n, rounded up (ceiling).
 */
static inline uint32_t ceil_log2(uint32_t n)
{
    if (unlikely(n == 0)) {
        __builtin_trap();
    }

    return (sizeof(uint32_t) * 8) - __builtin_clz(n - 1);
}

#endif /* MATH_H */
