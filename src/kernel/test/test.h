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
 *     make RUNTEST=<testname> kernel
 *
 * This will generate a build variant just for running that test.
 *
 * TODO(jlam55555): Interactively running tests.
 * TODO(jlam55555): Specify which tests to run.
 * TODO(jlam55555): Multiple "safety" levels for tests.
 */
#ifndef TEST_TEST_H
#define TEST_TEST_H

#ifdef TEST
#include <assert.h>

#include "common/libc.h"

// TODO(jlam55555): Working here.
#define TEST_ASSERT(cond)                                                      \
  if (!(cond)) {                                                               \
    printf("ASSERTION FAILED (%s:%u): %s\r\n", __FILE__, __LINE__, #cond);     \
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
void run_tests(void);

#endif

#endif // TEST_TEST_H
