/**
 * Very simple test runner. To define a new integration or unit test, create a
 * new file in this directory called "mytestname_test.cc". The file should look
 * like:
 *
 *    #include "test/test.h"
 *
 *    DEFINE_TEST(namespace, foo) {
 *        TEST_ASSERT(1 == 2);
 *    }
 *
 *    DEFINE_TEST(namespace1, namespace2, bar) {
 *        TEST_ASSERT(3 == 3);
 *    }
 *
 * This will generate tests named "namespace.foo" and
 * "namespace1.namespace2.bar", respectively.
 *
 * For unit tests of internal/private functions, define the tests in the same
 * file as the functions to test.
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
 * - pattern="^ns.": Match any test in namespace "ns".
 *
 * Test names must only include characters that form valid identifiers: letters,
 * digits, and underscores. Namespaces are denoted using periods. (You could
 * also create a separate namespacing system using e.g., double underscores, but
 * I don't foresee this being useful on top of the existing namespace system.)
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

#define _define_test(test_name_id, test_name_str)                              \
  static void test_##test_name_id(bool *test_passed); /* fwd declaration */    \
  static const struct test_info test_info_##test_name_id                       \
      __attribute__((used, section("test_rodata"))) = {                        \
          .name = test_name_str,                                               \
          .fn = test_##test_name_id,                                           \
  };                                                                           \
  static void test_##test_name_id(bool *test_passed) /* fn body */

#define DEFINE_TEST(ns, ...)                                                   \
  _define_test(ns##__##__VA_ARGS__, #ns "." #__VA_ARGS__)

/**
 * Run all tests, or run tests with the given name.
 *
 * TODO(jlam55555): Determine how to specify tests.
 */
void run_tests(const char *selection);

#endif // TEST_TEST_H
