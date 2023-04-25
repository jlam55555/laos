/**
 * Small subset of the C stdlib.
 *
 * For now, implementations are provided on an
 * as-needs basis.
 */
#ifndef COMMON_LIBC_H
#define COMMON_LIBC_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

// char.h
bool isprint(char c);

// string.h
size_t strlen(char *);

// stdio.h
size_t printf(char *fmt, ...);
size_t sprintf(char *s, char *fmt, ...);
size_t snprintf(char *s, size_t n, char *fmt, ...);
size_t vprintf(char *fmt, va_list);
size_t vsprintf(char *s, char *fmt, va_list);
size_t vsnprintf(char *s, size_t n, char *fmt, va_list);

#endif // COMMON_LIBC_H
