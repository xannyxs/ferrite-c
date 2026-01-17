#ifndef PRINTK_H
#define PRINTK_H

#include <ferrite/types.h>

__attribute__((target("general-regs-only"), format(printf, 1, 2))) int
printk(char const* fmt, ...);

#endif /* PRINTK_H */
