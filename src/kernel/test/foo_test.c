#include "test/test.h"

DEFINE_TEST(foo, foo) { TEST_ASSERT(1 != 2); }

DEFINE_TEST(foo, bar) {
  TEST_ASSERT(3 == 3);
  TEST_ASSERT(4 == 4);
  TEST_ASSERT(5 != 6);
}

DEFINE_TEST(foo, baz) {
  TEST_ASSERT(3 == 3);
  TEST_ASSERT(4 == 4);
  TEST_ASSERT(5 == 5);
}

DEFINE_TEST(foo, failing) { TEST_ASSERT(1 == 2); }
