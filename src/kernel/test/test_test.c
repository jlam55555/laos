/**
 * Tests for the test subsystem. In particular, the test pattern matcher, and
 * some example tests.
 */

#include "test/test.h"

DEFINE_TEST(test, foo) { TEST_ASSERT(1 != 2); }

DEFINE_TEST(test, bar) {
  TEST_ASSERT(3 == 3);
  TEST_ASSERT(4 == 4);
  TEST_ASSERT(5 != 6);
}

DEFINE_TEST(test, baz) {
  TEST_ASSERT(3 == 3);
  TEST_ASSERT(4 == 4);
  TEST_ASSERT(5 == 5);
}

DEFINE_TEST(test, simple_failing) { TEST_ASSERT(1 == 2); }

DEFINE_TEST(test, matches_simple) {
  const char *pat1 = "test.";
  const char *pat2 = "";
  const char *pat3 = "^test.";
  const char *pat4 = "pattern_matcher";
  const char *pat5 = "pattern_matcher$";
  const char *pat6 = "test.pattern_matcher$";
  const char *pat7 = "^test.pattern_matcher$";
  const char *pat8 = "^test.pattern_matcher";
  const char *pat9 = "^1test.pattern_matcher";
  const char *pat10 = "^test.pattern_matcher1";
  const char *pat11 = "test.pattern_matcher1$";
  const char *pat12 = "test.pattern_matcher1";
  const char *pat13 = ".";
  const char *pat14 = "match";

  const struct test_info test1 = {.name = "test.pattern_matcher"};

  TEST_ASSERT(test_matches(&test1, pat1));
  TEST_ASSERT(test_matches(&test1, pat2));
  TEST_ASSERT(test_matches(&test1, pat3));
  TEST_ASSERT(test_matches(&test1, pat4));
  TEST_ASSERT(test_matches(&test1, pat5));
  TEST_ASSERT(test_matches(&test1, pat6));
  TEST_ASSERT(test_matches(&test1, pat7));
  TEST_ASSERT(test_matches(&test1, pat8));
  TEST_ASSERT(!test_matches(&test1, pat9));
  TEST_ASSERT(!test_matches(&test1, pat10));
  TEST_ASSERT(!test_matches(&test1, pat11));
  TEST_ASSERT(!test_matches(&test1, pat12));
  TEST_ASSERT(test_matches(&test1, pat13));
  TEST_ASSERT(test_matches(&test1, pat14));

  const struct test_info test2 = {.name = "test.foo"};

  TEST_ASSERT(test_matches(&test2, pat1));
  TEST_ASSERT(test_matches(&test2, pat2));
  TEST_ASSERT(test_matches(&test2, pat3));
  TEST_ASSERT(!test_matches(&test2, pat4));
  TEST_ASSERT(!test_matches(&test2, pat5));
  TEST_ASSERT(!test_matches(&test2, pat6));
  TEST_ASSERT(!test_matches(&test2, pat7));
  TEST_ASSERT(!test_matches(&test2, pat8));
  TEST_ASSERT(!test_matches(&test2, pat9));
  TEST_ASSERT(!test_matches(&test2, pat10));
  TEST_ASSERT(!test_matches(&test2, pat11));
  TEST_ASSERT(!test_matches(&test2, pat12));
  TEST_ASSERT(test_matches(&test2, pat13));
  TEST_ASSERT(!test_matches(&test2, pat14));

  const struct test_info test3 = {.name = "bar.hello"};

  TEST_ASSERT(!test_matches(&test3, pat1));
  TEST_ASSERT(test_matches(&test3, pat2));
  TEST_ASSERT(!test_matches(&test3, pat3));
}

DEFINE_TEST(test, matches_disjunctive_simple_patterns) {
  const char *pat1 = "foo";
  const char *pat2 = "bar";
  const char *pat3 = "foo,bar";

  const struct test_info test1 = {.name = "foo"};
  const struct test_info test2 = {.name = "bar"};

  TEST_ASSERT(test_matches(&test1, pat1));
  TEST_ASSERT(!test_matches(&test1, pat2));
  TEST_ASSERT(test_matches(&test1, pat3));

  TEST_ASSERT(!test_matches(&test2, pat1));
  TEST_ASSERT(test_matches(&test2, pat2));
  TEST_ASSERT(test_matches(&test2, pat3));
}

DEFINE_TEST(test, matches_disjunctive_complex_patterns) {
  const char *pat1 = "^foo.hi$";
  const char *pat2 = "ba";
  const char *pat3 = "^foo.hi$,ba";

  const struct test_info test1 = {.name = "foo.hi"};
  const struct test_info test2 = {.name = "bar"};

  TEST_ASSERT(test_matches(&test1, pat1));
  TEST_ASSERT(!test_matches(&test1, pat2));
  TEST_ASSERT(test_matches(&test1, pat3));

  TEST_ASSERT(!test_matches(&test2, pat1));
  TEST_ASSERT(test_matches(&test2, pat2));
  TEST_ASSERT(test_matches(&test2, pat3));
}
