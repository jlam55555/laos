#ifndef LIBC_H
#define LIBC_H

#include <stdarg.h>

size_t strlen(char *);

size_t printf(char *fmt, ...);
size_t sprintf(char *s, char *fmt, ...);
size_t snprintf(char *s, size_t n, char *fmt, ...);
size_t vprintf(char *fmt, va_list);
size_t vsprintf(char *s, char *fmt, va_list);
size_t vsnprintf(char *s, size_t n, char *fmt, va_list);

#endif // LIBC_H
