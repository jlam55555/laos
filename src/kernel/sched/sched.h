/**
 * Very simple round-robin task scheduler.
 *
 * TODO(jlam55555): Set up unit tests.
 * TODO(jlam55555): Set up quanta.
 * TODO(jlam55555): Document idle task.
 */
#ifndef SCHED_SCHED_H
#define SCHED_SCHED_H

#include "common/list.h"

/**
 * Round-robin scheduler.
 */
struct sched_task;
struct scheduler {
  struct list_head running;
  struct list_head runnable;
  struct list_head blocked;

  struct sched_task *current_task;
};

struct sched_task {
  struct list_head *ll;
  struct scheduler *parent;
  void *stk;
};

/**
 * Creates a task in the given scheduler.
 */
struct task *sched_create_task(struct scheduler *scheduler);

/**
 * Destroy task. If this is the current task in the scheduler, then schedule
 * away.
 */
struct sched_task_destroy(struct sched_task *task);

/**
 * Choose the next task to schedule (but don't actually schedule). Behavior:
 *
 * - If the runnable queue is not empty, return any task on it.
 * - Return the current task.
 *
 * Note that the current task shouldn't be blocked. It's an error if it is,
 * because there should always be an "idle" task that is always
 * running/runnable (never blocked).
 */
struct task *sched_choose_task(struct scheduler *scheduler);

/**
 * Switch to the given task. This comprises of a few steps:
 *
 * If the task is already running, nothing to do.
 *
 * If the task is not already running:
 * - Make sure the given task is not blocked or running (it must be runnable).
 * - If the currently-running task is running (i.e., not blocked), set it to
 *   runnable. (We pre-empt it.)
 * - Save the state of the previous task onto its stack.
 * - Set the scheduled task to running.
 * - Restore the stack of the given task.
 */
struct task *sched_task_switch(struct sched_task *task);

#endif // SCHED_SCHED_H
