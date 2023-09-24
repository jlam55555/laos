#include "sched/sched.h"

#include "common/libc.h"
#include "test/test.h"

struct scheduler scheduler;

void init_fn() {
  for (int i = 0; i < 5; ++i) {
    // TODO(jlam55555): This printing is not very intuitive because all these
    // threads have the same printing. Need threads to distinguish themselves
    // somehow, e.g., a name.
    printf("Thread 2: %d Running in a thread!\r\n", i);
    sched_task_switch(sched_choose_task(&scheduler));
  }

  sched_task_destroy(scheduler.current_task);
}

/**
 * Simple cooperative threading.
 *
 * TODO(jlam55555): May not be cooperative after preemption is enabled on timer
 * interrupt. Will probably need to disable interrupts for that.
 */
DEFINE_TEST(sched, coroutines) {
  // TODO(jlam55555): This is okay as a standalone test, but there is no way to
  // "get out of the scheduler" yet, in the same way that initializing virtual
  // memory is a one-way process. Also, we cannot virtualize scheduling within
  // scheduling easily. So we need a way of isolating this test. Note also that
  // `sched_task_destroy()` always calls the bottom half scheduler, whereas we
  // can manually call just the top-half `sched_task_switch_nostack()`. We
  // should make this test only call the top-half schedulers and not do the
  // bootstrapping. It's probably pretty hard to unit test the bootstrapping or
  // the bottom half (non-bookkeeping) in general.
  //
  // TODO(jlam55555): This test also tests multiple things at the moment, such
  // as coroutines and scheduling itself when there are no other runnable
  // processes.
  sched_init(&scheduler);

  for (int i = 0; i < 5; ++i) {
    sched_create_task(&scheduler, init_fn);
  }
  sched_bootstrap_task(&scheduler);

  for (int i = 0; i < 10; ++i) {
    printf("Thread 1: %d Running in a thread!\r\n", i);
    sched_task_switch(sched_choose_task(&scheduler));
  }

  // Trying to destroy the last task will fail.
  //
  // sched_task_destroy(scheduler.current_task);

  TEST_ASSERT(true);
}
