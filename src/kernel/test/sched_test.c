#include "sched/sched.h"

#include "common/list.h"
#include "test/test.h"

/**
 * Test basic lifecycle, including startup and destroyed state (`sched_init()`
 * and `sched_destroy()`).
 */
DEFINE_TEST(sched, lifecycle) {
  struct scheduler scheduler;

  sched_init(&scheduler);

  // Initial state.
  TEST_ASSERT(list_empty(&scheduler.runnable));
  TEST_ASSERT(list_empty(&scheduler.blocked));
  TEST_ASSERT(!scheduler.current_task);

  // Create some tasks.
  struct sched_task *tasks[10];
  for (int i = 0; i < 10; ++i) {
    TEST_ASSERT(tasks[i] = sched_create_task(&scheduler, NULL));
  }

  // Start a task.
  TEST_ASSERT(!scheduler.current_task);
  sched_task_switch_nostack(tasks[0]);
  TEST_ASSERT(scheduler.current_task);

  // Do some task switching.
  for (int i = 9; i >= 0; --i) {
    sched_task_switch_nostack(tasks[i]);
  }

  /* TEST_ASSERT(scheduler.current_task); */
  sched_destroy(&scheduler);

  // Destroyed state.
  TEST_ASSERT(!scheduler.current_task);
  TEST_ASSERT(list_empty(&scheduler.runnable));
  TEST_ASSERT(list_empty(&scheduler.blocked));
}

/**
 * Test task selection.
 */
DEFINE_TEST(sched, choose_task) {
  struct scheduler scheduler;
  sched_init(&scheduler);

  struct sched_task *task1, *task2;
  TEST_ASSERT(task1 = sched_create_task(&scheduler, NULL));
  TEST_ASSERT(task2 = sched_create_task(&scheduler, NULL));

  // Set task1 as initial task. (Bootstrap process.)
  sched_task_switch_nostack(task1);
  TEST_ASSERT(scheduler.current_task == task1);

  // Choose a new task. It should select task2 since it's runnable.
  TEST_ASSERT(sched_choose_task(&scheduler) == task2);

  // Choose a new task when there are no runnable tasks available. It should
  // choose the current (running) task.
  sched_task_destroy_nostack(task2);
  TEST_ASSERT(sched_choose_task(&scheduler) == task1);

  sched_destroy(&scheduler);
}

/**
 * Test that task selection is round-robin when there are multiple runnable
 * tasks.
 */
DEFINE_TEST(sched, choose_task_rr) {
  struct scheduler scheduler;
  sched_init(&scheduler);

  // Create N tasks.
  struct sched_task *tasks[4], *next_task;
  for (int i = 0; i < 4; ++i) {
    TEST_ASSERT(tasks[i] = sched_create_task(&scheduler, NULL));
  }

  // Schedule tasks in some arbitrary order.
  sched_task_switch_nostack(tasks[3]);
  sched_task_switch_nostack(tasks[1]);
  sched_task_switch_nostack(tasks[0]);
  sched_task_switch_nostack(tasks[2]);

  // Schedule N times using `sched_choose_task()`. Make sure they appear in the
  // same order.
  //
  // running task
  // |
  // |   runnable tasks
  // v   v
  // 2 | 3 1 0
  // 3 | 1 0 2
  // 1 | 0 2 3
  // 0 | 2 3 1
  // 2 | 3 1 0
  // 3 | 1 0 2
  TEST_ASSERT((next_task = sched_choose_task(&scheduler)) == tasks[3]);
  sched_task_switch_nostack(next_task);
  TEST_ASSERT((next_task = sched_choose_task(&scheduler)) == tasks[1]);
  sched_task_switch_nostack(next_task);
  TEST_ASSERT((next_task = sched_choose_task(&scheduler)) == tasks[0]);
  sched_task_switch_nostack(next_task);
  TEST_ASSERT((next_task = sched_choose_task(&scheduler)) == tasks[2]);
  sched_task_switch_nostack(next_task);
  TEST_ASSERT((next_task = sched_choose_task(&scheduler)) == tasks[3]);
  sched_task_switch_nostack(next_task);

  // Throw some additions/deletions into the mix.
  //
  // 3 | 0 2
  // 0 | 2 3
  // 2 | 3 0
  // 3 | 0 2
  // 0 | 2 3
  // 2 | 3 0
  sched_task_destroy_nostack(tasks[1]);
  TEST_ASSERT((next_task = sched_choose_task(&scheduler)) == tasks[0]);
  sched_task_switch_nostack(next_task);
  TEST_ASSERT((next_task = sched_choose_task(&scheduler)) == tasks[2]);
  sched_task_switch_nostack(next_task);
  TEST_ASSERT((next_task = sched_choose_task(&scheduler)) == tasks[3]);
  sched_task_switch_nostack(next_task);
  TEST_ASSERT((next_task = sched_choose_task(&scheduler)) == tasks[0]);
  sched_task_switch_nostack(next_task);
  TEST_ASSERT((next_task = sched_choose_task(&scheduler)) == tasks[2]);
  sched_task_switch_nostack(next_task);

  // 2 | 3 0 1
  // 3 | 0 1 2
  // 1 | 2 3 0
  // 2 | 3 0 1
  // 3 | 0 1 2
  tasks[1] = sched_create_task(&scheduler, NULL);
  TEST_ASSERT((next_task = sched_choose_task(&scheduler)) == tasks[3]);
  sched_task_switch_nostack(next_task);
  TEST_ASSERT((next_task = sched_choose_task(&scheduler)) == tasks[0]);
  sched_task_switch_nostack(next_task);
  TEST_ASSERT((next_task = sched_choose_task(&scheduler)) == tasks[1]);
  sched_task_switch_nostack(next_task);
  TEST_ASSERT((next_task = sched_choose_task(&scheduler)) == tasks[2]);
  sched_task_switch_nostack(next_task);
  TEST_ASSERT((next_task = sched_choose_task(&scheduler)) == tasks[3]);

  sched_destroy(&scheduler);
}

/**
 * Test task switching. (This doesn't test the actual stack switching, which is
 * a bit hard to test, so the _nostack() version is used here. This means we
 * only test the bookkeeping part of the task switching.)
 */
DEFINE_TEST(sched, task_switch) {
  struct scheduler scheduler;
  sched_init(&scheduler);

  struct sched_task *task1, *task2;
  TEST_ASSERT(task1 = sched_create_task(&scheduler, NULL));
  TEST_ASSERT(task2 = sched_create_task(&scheduler, NULL));

  // Initial state.
  TEST_ASSERT(task1->state != SCHED_RUNNING);
  TEST_ASSERT(scheduler.current_task != task1);
  TEST_ASSERT(task2->state != SCHED_RUNNING);
  TEST_ASSERT(scheduler.current_task != task2);

  // Initial bootstrap to task1.
  sched_task_switch_nostack(task1);
  TEST_ASSERT(task1->state == SCHED_RUNNING);
  TEST_ASSERT(scheduler.current_task == task1);
  TEST_ASSERT(task2->state != SCHED_RUNNING);
  TEST_ASSERT(scheduler.current_task != task2);

  // Switch to a different (runnable) task.
  sched_task_switch_nostack(task2);
  TEST_ASSERT(task1->state != SCHED_RUNNING);
  TEST_ASSERT(scheduler.current_task != task1);
  TEST_ASSERT(task2->state == SCHED_RUNNING);
  TEST_ASSERT(scheduler.current_task == task2);

  // Switch to same (runnable) task. This should be a no-op/idempotent.
  sched_task_switch_nostack(task2);
  TEST_ASSERT(task1->state != SCHED_RUNNING);
  TEST_ASSERT(scheduler.current_task != task1);
  TEST_ASSERT(task2->state == SCHED_RUNNING);
  TEST_ASSERT(scheduler.current_task == task2);

  sched_destroy(&scheduler);
}

/**
 * Similar to the above, we only test the bookkeeping side of things by using
 * the _nostack() variant. (This means that we won't switch stacks if the
 * current running task is destroyed.)
 *
 * Note that we cannot do any assertions on the deleted task because it will
 * have been freed by the scheduler.
 */
DEFINE_TEST(sched, task_destroy) {
  struct scheduler scheduler;
  sched_init(&scheduler);

  struct sched_task *task1, *task2, *task3;
  TEST_ASSERT(task1 = sched_create_task(&scheduler, NULL));
  TEST_ASSERT(task2 = sched_create_task(&scheduler, NULL));
  TEST_ASSERT(task3 = sched_create_task(&scheduler, NULL));

  sched_task_switch_nostack(task1);
  TEST_ASSERT(scheduler.current_task == task1);
  TEST_ASSERT(task1->state == SCHED_RUNNING);
  TEST_ASSERT(task2->state == SCHED_RUNNABLE);
  TEST_ASSERT(task3->state == SCHED_RUNNABLE);

  // Delete a task that is not running.
  sched_task_destroy_nostack(task2);
  TEST_ASSERT(scheduler.current_task == task1);
  TEST_ASSERT(task1->state == SCHED_RUNNING);
  TEST_ASSERT(task3->state == SCHED_RUNNABLE);

  // Delete the running task. See that we've switched away to a runnable task.
  sched_task_destroy_nostack(task1);
  TEST_ASSERT(scheduler.current_task == task3);
  TEST_ASSERT(task3->state == SCHED_RUNNING);

  // See that we have no other runnable tasks remaining since we've deleted
  // them.
  TEST_ASSERT(list_empty(&scheduler.runnable));

  sched_destroy(&scheduler);
}
