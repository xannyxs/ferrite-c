#ifndef _STDLIB_H
#define _STDLIB_H 1

__attribute__((__noreturn__)) void abort(char*);

/*
 * WARNING: 'atol' used to convert a string to an integer value, but function
 * will not report conversion errors; consider using 'strtol' instead
 */
long atol(char const*);

#endif
