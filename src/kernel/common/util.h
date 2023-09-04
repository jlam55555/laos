#ifndef COMMON_UTIL_H
#define COMMON_UTIL_H

#include <stdint.h>

// Read the hardware timestamp counter. We want this to be
// inline to decrease overhead. It may be better to have
// this as a pure assembly function, but inline assembly
// shouldn't be too far off.
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
#define JOIN3(delim, a, ...) a delim JOIN2(__VA_ARGS__)
#define JOIN4(delim, a, ...) a delim JOIN3(__VA_ARGS__)
#define JOIN5(delim, a, ...) a delim JOIN4(__VA_ARGS__)
#define JOIN6(delim, a, ...) a delim JOIN5(__VA_ARGS__)
#define JOIN7(delim, a, ...) a delim JOIN6(__VA_ARGS__)
#define JOIN8(delim, a, ...) a delim JOIN7(__VA_ARGS__)

/** Stringify a macro. E.g., one that comes from a -Dmacro=xyz gcc arg.
 * https://gcc.gnu.org/onlinedocs/cpp/Stringizing.html
 */
#define _macro2str(macro...) #macro
#define macro2str(macro) _macro2str(macro)

#endif // COMMON_UTIL_H
