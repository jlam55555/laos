#ifndef COMMON_UTIL_H
#define COMMON_UTIL_H

#include <stdint.h>

/**
 * Read the hardware timestamp counter. We want this to be inline to decrease
 * overhead. It may be better to have this as a pure assembly function, but
 * inline assembly shouldn't be too far off.
 */
inline uint64_t read_tsc(void) {
  uint64_t res;
  __asm__ volatile("rdtsc\n\t"          // Returns the time in EDX:EAX.
                   "shl $32, %%rdx\n\t" // Shift the upper bits left.
                   "or %%rdx, %0"       // 'Or' in the lower bits.
                   : "=a"(res)
                   :
                   : "rdx");
  return res;
}

inline void outb(uint8_t value, uint16_t port) {
  __asm__("outb %[value], %[port]" : : [value] "a"(value), [port] "Nd"(port));
}

inline void outw(uint16_t value, uint16_t port) {
  __asm__("outw %[value], %[port]" : : [value] "a"(value), [port] "Nd"(port));
}

inline uint8_t inb(uint16_t port) {
  uint8_t rv;
  __asm__ volatile("inb %1, %0" : "=a"(rv) : "d"(port));
  return rv;
}

/**
 * Join strings with a delimiter. Add more as necessary. All parameters should
 * be string literals.
 *
 * Note that the end result is not parenthesized into a single expression.
 */
#define JOIN2(delim, a, b) a delim b
#define JOIN3(delim, a, ...) a delim JOIN2(delim, __VA_ARGS__)
#define JOIN4(delim, a, ...) a delim JOIN3(delim, __VA_ARGS__)
#define JOIN5(delim, a, ...) a delim JOIN4(delim, __VA_ARGS__)
#define JOIN6(delim, a, ...) a delim JOIN5(delim, __VA_ARGS__)
#define JOIN7(delim, a, ...) a delim JOIN6(delim, __VA_ARGS__)
#define JOIN8(delim, a, ...) a delim JOIN7(delim, __VA_ARGS__)

/**
 * Stringify a macro. E.g., one that comes from a -Dmacro=xyz gcc arg.
 * https://gcc.gnu.org/onlinedocs/cpp/Stringizing.html
 */
#define _macro2str(macro...) #macro
#define macro2str(macro) _macro2str(macro)

/**
 * ilog2(n) = floor(log_2(n))
 */
static inline int ilog2(uint32_t n) {
  signed rv;
  if (!n) {
    return -1;
  }
  __asm__("bsr %1, %0" : "=r"(rv) : "r"(n));
  return rv;
}

/**
 * ilog2ceil(n) = ceil(log_2(n)). Example usage: rounding to the next higher
 * power of two, but you want to get the exponent.
 */
static inline int ilog2ceil(uint32_t n) { return n ? ilog2(n - 1) + 1 : -1; }

/**
 * _Static_assert is a C11 extension. Make it look like the C++11/C23 syntax.
 */
#define static_assert _Static_assert

#endif // COMMON_UTIL_H
