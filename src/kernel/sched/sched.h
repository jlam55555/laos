/**
 * Very simple round-robin task scheduler.
 *
 * TODO(jlam55555): Set up unit tests.
 * TODO(jlam55555): Set up quanta.
 * TODO(jlam55555): Document idle task.
 * TODO(jlam55555): Document scheduler "valid" state. Only after calling
 * `sched_task_switch_nostack()` for the first time will there be a current task
 * in a scheduler. Similarly, after calling `sched_destroy()` there will be no
 * more tasks on this scheduler.
 */
#ifndef SCHED_SCHED_H
#define SCHED_SCHED_H

#include "common/list.h"

/**
 * Round-robin scheduler.
 */
struct sched_task;
struct scheduler {
  struct list_head runnable;
  struct list_head blocked;

  struct sched_task *current_task;
};

/**
 * TODO(jlam55555): Document this.
 * TODO(jlam55555): Allow tasks to have a desc/name.
 */
struct sched_task {
  struct list_head ll;
  struct scheduler *parent;
  void *stk;
  enum sched_task_state {
    SCHED_RUNNING,
    SCHED_RUNNABLE,
    SCHED_BLOCKED,
  } state;
};

/**
 * Initialize scheduler. Note that no idle task is created, you have to create
 * one yourself.
 *
 * To enter the scheduler, you need to do two things:
 *
 * 1. Create an idle task.
 * 2. Mark the current task as part of the scheduler using
 *    `sched_bootstrap_task()`.
 */
void sched_init(struct scheduler *scheduler);

/**
 * Creates a task in the given scheduler.
 */
struct sched_task *sched_create_task(struct scheduler *scheduler,
                                     void (*cb)(struct sched_task *));

/**
 * Add the current thread to the given scheduler. This is used for the
 * bootstrapping process, and should be called exactly once.
 */
void sched_bootstrap_task(struct scheduler *scheduler);

/**
 * Top half of `sched_task_destroy()`. Doesn't switch stacks. Exposed for unit
 * testing.
 */
void sched_task_destroy_nostack(struct sched_task *task);

/**
 * Destroy task. If this is the current task in the scheduler, then schedule
 * away.
 */
void sched_task_destroy(struct sched_task *task);

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
struct sched_task *sched_choose_task(struct scheduler *scheduler);

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
 * - Jump to the new instruction pointer.
 */
void sched_task_switch(struct sched_task *task);

/**
 * Similar to `sched_task_switch()`, but don't switch stacks or jump instruction
 * pointer -- this is basically the bookkeeping part of task switching. This is
 * useful for unit-testing the scheduler and as a building block in
 * `sched_task_switch()`.
 *
 * This has special behavior for the initial "bootstrap" call, in which there is
 * no old task to be torn down.
 */
void sched_task_switch_nostack(struct sched_task *task);

/**
 * Tear down a scheduler and all tasks associated with it.
 *
 * Note that if those tasks allocated any memory, those are not tracked and
 * would not be freed.
 */
void sched_destroy(struct scheduler *scheduler);

#endif // SCHED_SCHED_H
