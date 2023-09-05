#include "common/util.h"

#include "common/libc.h"
#include "test/test.h"

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
