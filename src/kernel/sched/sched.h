/**
 * Very simple round-robin task scheduler for kernel threads. This does not have
 * any userspace thread/process semantics.
 *
 * Each task ("thread") has its own stack, instruction pointer (implicitly
 * stored on the stack when scheduling away), and no memory protections. A
 * scheduler will have exactly one task scheduled (running) at a time (after the
 * bootstrap process and before destruction).
 *
 * To perform scheduling, you must first initialize a scheduler object. Tasks
 * can be added to this scheduler (either via `sched_clone_task()` or
 * `sched_create_task()`, which are like Linux's `clone()` and `exec*()`
 * syscalls, respectively). Tasks can be scheduled using `sched_choose_task()`
 * and `sched_task_switch()`.
 *
 * In general, you will not need to worry about creating a scheduler object, the
 * idle task, or bootstrapping process -- the interfaces to initialize a
 * scheduler are mostly for unit testing. There is a global main task scheduler
 * which all scheduling operations are implied to act on.
 *
 * This kernel uses pre-emptive scheduling. I.e., the timer interrupt will force
 * a process to call `schedule()` if it has exceeded its quantum.
 *
 * TODO(jlam55555): Set up quanta/pre-emptive scheduling.
 * TODO(jlam55555): Set up global main scheduler.
 * TODO(jlam55555): Implement `clone()` API.
 *
 * =============================================================================
 * Bootstrapping process
 * =============================================================================
 * There is some special handling around entering the scheduler for the first
 * time -- this is called the "bootstrapping process". There are two ways to do
 * this. Calling `sched_task_switch(_nostack)()` for the first time will
 * automatically bootstrap the given thread if there were no previously running
 * tasks. However, doing so will also discard the current stack, which may be
 * undesirable -- thus, the helper function `sched_bootstrap_task()` is provided
 * to form an `exec*()`-like interface to enter the scheduler for the first
 * time. This is used in the global main task scheduler.
 *
 * Bootstrapping a scheduler when another one is already running, or destroying
 * a bootstrapped scheduler are both UB -- we expect that there is exactly one
 * kernel-level task scheduler that persists the entire lifetime of the
 * operating system once it has bootstrapped. (This is not the case for unit
 * tests, which use a pseudo-bootstrapping process. This may also change in the
 * future if scheduling needs become more complicated.)
 *
 * =============================================================================
 * Idle task
 * =============================================================================
 * In general, the round-robin scheduler chooses the first task on the runnable
 * queue to schedule. If not, then it'll choose the previously-running task, if
 * it is not in a blocked state (i.e., it was pre-empted). But what if there are
 * no runnable tasks and the previously-running task is blocked?
 *
 * This motivates an idle task, which is a regular task with some special
 * properties: it always exists and is always runnable/running (never blocked).
 * It will thus be the target for scheduling if no other task is schedulable.
 * Like a sentinel node, this more cleanly handles this edge case than trying to
 * write the logic to manually wait for a task to be schedulable.
 *
 * The idle task is not provided by the scheduler -- the implementor must create
 * the idle task. Here is an example of how to bootstrap into the scheduler with
 * an idle task:
 *
 * ```
 * static void _idle_task(void) { for (;;) { __asm__("hlt"); } }
 * ...
 * struct scheduler scheduler;
 * sched_add_task(&scheduler, _idle_task);
 * sched_bootstrap_task(&scheduler);
 * ```
 */
#ifndef SCHED_SCHED_H
#define SCHED_SCHED_H

#include "common/list.h"

/**
 * Round-robin task scheduler.
 */
struct sched_task;
struct scheduler {
  struct list_head runnable;
  struct list_head blocked;

  struct sched_task *current_task;
};

/**
 * A kernel thread.
 *
 * TODO(jlam55555): Allow threads to have a name.
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
 *
 * Note that the scheduler manages allocation rather than accepting a `struct
 * sched_task *`. This is so that the scheduler also manages the destruction/
 * freeing of all tasks.
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
 *
 * Note that the destroyed task will be freed, so you cannot use it afterwards.
 */
void sched_task_destroy(struct sched_task *task);

/**
 * Choose the next task to schedule (but don't actually schedule). Behavior:
 *
 * - If the runnable queue is not empty, return the first task on it.
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
