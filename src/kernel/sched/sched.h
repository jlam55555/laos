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
  struct list_head runnable;
  struct list_head blocked;

  struct sched_task *current_task;
};

struct sched_task {
  struct list_head ll;
  struct scheduler *parent;
  void *ip;
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

#endif // SCHED_SCHED_H
