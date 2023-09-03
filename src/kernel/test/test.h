/**
 * Very simple test runner. To define a new test, create a new file in this
 * directory called "mytestname_test.cc". The file should look like:
 *
 *    #include "test/test.h"
 *
 *    DEFINE_TEST(foo) {
 *        TEST_ASSERT(1 == 2);
 *    }
 *
 *    DEFINE_TEST(bar) {
 *        TEST_ASSERT(3 == 3);
 *    }
 *
 * Automatic test discovery based on:
 * https://gist.github.com/nickrolfe/ffc9b1c02381b9dc17c975b98db42172
 *
 * Build a kernel that only runs a test after initialization using:
 *
 *     make RUNTEST=<test_selection> kernel
 *
 * This will generate a build variant just for running that test. Note that test
 * selection occurs at runtime, not compile-time, so the build may succeed but
 * test selection may fail.
 *
 * Test selection follows a very simple pattern matcher:
 * - pattern="": Empty pattern matches everything.
 * - pattern="foo": Match any test whose name contains "foo".
 * - pattern="^bar": Match any test whose name starts with "bar".
 * - pattern="baz$": Match any test whose name ends with "baz".
 * - pattern="pat1,pat2": Match any test which matches either pat1 or pat2.
 *
 * Test names must only include characters that form valid identifiers: letters,
 * digits, and underscores. Double underscores can be used for namespacing:
 * e.g., the "selection matcher" test within the testing module may be indicated
 * by a test whose name is "test__selection_matcher". Matching all tests within
 * a namespace "foonamespace" can be done with the pattern "^foonamespace__".
 *
 * All lines printed by the test subsystem are prefixed by TEST_PREFIX. This is
 * intended to make the plaintext output heuristically-parsable by an external
 * script, and should be made unique enough to avoid likely collisions with
 * test stdout logging.
 *
 * TODO(jlam55555): Multiple "safety" levels for tests.
 */
#ifndef TEST_TEST_H
#define TEST_TEST_H

#include <assert.h>

#include "common/libc.h"

#define TEST_PREFIX "##!! "

#define TEST_ASSERT(cond)                                                      \
  if (!(cond)) {                                                               \
    printf(TEST_PREFIX "ASSERTION FAILED (%s:%u): %s\r\n", __FILE__, __LINE__, \
           #cond);                                                             \
    *test_passed = false;                                                      \
    return;                                                                    \
  }

typedef void (*test_fn)(bool *);

/**
 * Test descriptor for automatic test detection.
 */
struct test_info {
  const char *name;
  test_fn fn;
};

#define DEFINE_TEST(test_name)                                                 \
  static void test_##test_name(bool *test_passed); /* fwd declaration */       \
  static const struct test_info test_info_##test_name                          \
      __attribute__((used, section("test_rodata"))) = {                        \
          .name = #test_name,                                                  \
          .fn = test_##test_name,                                              \
  };                                                                           \
  static void test_##test_name(bool *test_passed) /* fn body */

/**
 * Run all tests, or run tests with the given name.
 *
 * TODO(jlam55555): Determine how to specify tests.
 */
void run_tests(const char *selection);

#endif // TEST_TEST_H
