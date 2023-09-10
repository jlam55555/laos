#include "common/util.h"

#include "common/libc.h"
#include "test/test.h"

#include <limits.h>
#include <stdint.h>

DEFINE_TEST(util, join) {
  TEST_ASSERT(!strcmp(JOIN2(",", "a", "b"), "a,b"));
  TEST_ASSERT(!strcmp(JOIN4(",", "a", "b", "c", "d"), "a,b,c,d"));
  TEST_ASSERT(!strcmp(JOIN8(",", "a", "b", "c", "d", "e", "f", "g", "h"),
                      "a,b,c,d,e,f,g,h"));
  TEST_ASSERT(!strcmp(JOIN4("-.-", "bca", "fad", "cat", "bag"),
                      "bca-.-fad-.-cat-.-bag"));
}

DEFINE_TEST(util, macro2str) {
#define TEST_MACRO1 abcde
#define TEST_MACRO2 a, ^b$
  TEST_ASSERT(!strcmp(macro2str(TEST_MACRO1), "abcde"));
  TEST_ASSERT(!strcmp(macro2str(TEST_MACRO2), "a, ^b$"));
#undef TEST_MACRO1
#undef TEST_MACRO2
}

DEFINE_TEST(util, ilog2) {
  TEST_ASSERT(ilog2(0) == -1);
  TEST_ASSERT(ilog2(1) == 0);
  TEST_ASSERT(ilog2(2) == 1);
  TEST_ASSERT(ilog2(3) == 1);
  TEST_ASSERT(ilog2(4) == 2);
  TEST_ASSERT(ilog2(5) == 2);
  TEST_ASSERT(ilog2(7) == 2);
  TEST_ASSERT(ilog2(8) == 3);
  TEST_ASSERT(ilog2(9) == 3);
  TEST_ASSERT(ilog2(UINT32_MAX >> 1) == 30);
  TEST_ASSERT(ilog2((UINT32_MAX >> 1) + 1) == 31);
  TEST_ASSERT(ilog2(UINT32_MAX - 1) == 31);
  TEST_ASSERT(ilog2(UINT32_MAX) == 31);
}

DEFINE_TEST(util, ilog2ceil) {
  TEST_ASSERT(ilog2ceil(0) == -1);
  TEST_ASSERT(ilog2ceil(1) == 0);
  TEST_ASSERT(ilog2ceil(2) == 1);
  TEST_ASSERT(ilog2ceil(3) == 2);
  TEST_ASSERT(ilog2ceil(4) == 2);
  TEST_ASSERT(ilog2ceil(5) == 3);
  TEST_ASSERT(ilog2ceil(7) == 3);
  TEST_ASSERT(ilog2ceil(8) == 3);
  TEST_ASSERT(ilog2ceil(9) == 4);
  TEST_ASSERT(ilog2ceil(UINT32_MAX >> 1) == 31);
  TEST_ASSERT(ilog2ceil((UINT32_MAX >> 1) + 1) == 31);
  TEST_ASSERT(ilog2ceil(UINT32_MAX - 1) == 32);
  TEST_ASSERT(ilog2ceil(UINT32_MAX) == 32);
}
