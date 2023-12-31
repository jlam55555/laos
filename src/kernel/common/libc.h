/**
 * Implementation of a small part of the C standard library, for kernel use. Not
 * intended to be standards-compliant.
 *
 * For now, implementations are provided on an as-needs basis.
 */
#ifndef COMMON_LIBC_H
#define COMMON_LIBC_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

// char.h
bool isprint(char c);

// string.h
int memcmp(const void *, const void *, size_t);
void *memcpy(void *__restrict, const void *__restrict, size_t);
void *memmove(void *, const void *, size_t);
void *memset(void *, int, size_t);
size_t strlen(const char *);
int strcmp(const char *, const char *);
int strncmp(const char *, const char *, size_t);

// stdio.h
size_t printf(const char *fmt, ...);
size_t sprintf(char *s, const char *fmt, ...);
size_t snprintf(char *s, size_t n, const char *fmt, ...);
size_t vprintf(const char *fmt, va_list);
size_t vsprintf(char *s, const char *fmt, va_list);
size_t vsnprintf(char *s, size_t n, const char *fmt, va_list);

#endif // COMMON_LIBC_H
