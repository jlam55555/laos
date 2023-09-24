#include "sched/sched.h"

#include "common/libc.h"
#include "test/test.h"

DEFINE_TEST(sched, lifecycle) {
  struct scheduler scheduler;

  sched_init(&scheduler);

  // Shouldn't be any active task yet.
  TEST_ASSERT(!scheduler.current_task);

  // Create some tasks.
  struct sched_task *tasks[10];
  for (int i = 0; i < 10; ++i) {
    tasks[i] = sched_create_task(&scheduler, NULL);
  }

  // Start a task.
  TEST_ASSERT(!scheduler.current_task);
  sched_task_switch_nostack(tasks[0]);
  TEST_ASSERT(scheduler.current_task);

  // Do some task switching.
  for (int i = 9; i >= 0; --i) {
    sched_task_switch_nostack(tasks[i]);
  }

  TEST_ASSERT(scheduler.current_task);
  sched_destroy(&scheduler);
  TEST_ASSERT(!scheduler.current_task);
}
