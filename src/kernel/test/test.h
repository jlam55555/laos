/**
 * Very simple test runner. To define a new test, create a new file in this
 * directory called "mytestname_test.cc". The file should look like:
 *
 *    #ifdef RUNTEST_MYTESTNAME
 *    #include "test/test.h"
 *
 *    void run_test(void) {
 *        assert(1 == 2);
 *    }
 *
 *    #endif // RUNTEST_MYTESTNAME
 *
 * TODO(jlam55555): Automatic test discovery. See
 * https://gist.github.com/nickrolfe/ffc9b1c02381b9dc17c975b98db42172
 */
#ifndef TEST_TEST_H
#define TEST_TEST_H

#ifdef TEST
#include <assert.h>

void run_test(void);

#endif

#endif // TEST_TEST_H
