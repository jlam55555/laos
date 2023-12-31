/**
 * Language tricks and other useful utilities, such as macro metaprogramming,
 * math tricks, and conventional aliases.
 */

#ifndef COMMON_UTIL_H
#define COMMON_UTIL_H

#include <stdint.h>

#include "common/opcodes.h" // for op_bsr

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
  // The output of `bsr` (on x86_64) is UB if the input is 0.
  return n ? (signed)op_bsr(n) : -1;
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
