#ifndef MATH_H
#define MATH_H

#define ALIGN(x, a) __ALIGN_MASK(x, (typeof(x))(a) - 1)
#define __ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))

#define CEIL_DIV(a, b) (((a + b - 1) / b))

#endif /* MATH_H */
