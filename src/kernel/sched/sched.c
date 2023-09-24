#include "sched/sched.h"

#include <assert.h>

#include "common/list.h"
#include "mem/phys.h"
#include "mem/slab.h"

void sched_init(struct scheduler *scheduler) {
  list_init(&scheduler->runnable);
  list_init(&scheduler->blocked);

  // No idle task.
  scheduler->current_task = NULL;
}

struct sched_task *sched_create_task(struct scheduler *scheduler,
                                     void (*cb)(struct sched_task *)) {
  // TODO(jlam55555): Allow specifying the allocator.
  // Actually, better yet to not allocate. Let the caller allocate.
  struct sched_task *task = kmalloc(sizeof(struct sched_task));
  if (!task) {
    return task;
  }

  // The stack should look like we're in the middle of the sched_task_switch
  // function.
  //
  // `cb` will be NULL for the bootstrap thread. We don't need to set up the
  // stack in that case.
  if (cb) {
    size_t *stk = phys_alloc_page();
    assert(stk);

    // Go to the top of the page.
    stk = (void *)stk + PG_SZ;

    // Push %rip.
    --stk;
    *stk = (size_t)cb;

    // Push %rbp, %rbx, %r12, %r13, %r14, %r15 (all garbage).
    stk -= 6;

    // Save the stack.
    task->stk = stk;
  }

  task->parent = scheduler;
  task->state = SCHED_RUNNABLE;
  list_add(&scheduler->runnable, &task->ll);
  return task;
}

void sched_bootstrap_task(struct scheduler *scheduler) {
  // `sched_create_task()` has special handling for the bootstrapping process
  // (callback is NULL).
  struct sched_task *task = sched_create_task(scheduler, NULL);

  // `sched_task_switch_nostack()` has special handling for the bootstrapping
  // process (no previous running task).
  sched_task_switch_nostack(task);
}

void sched_task_destroy(struct sched_task *task) {
  assert(task->parent);

  list_del(&task->ll);

  // If this is the running process, schedule away.
  //
  // TODO(jlam55555): The interfaces here are a bit wonky.
  if (task->parent->current_task == task) {
    sched_task_switch(sched_choose_task(task->parent));
  }

  // Free the stack. This assumes we didn't overflow the stack.
  phys_free_page(PG_FLOOR(task->stk));
  kfree(task);
}

struct sched_task *sched_choose_task(struct scheduler *scheduler) {
  if (!list_empty(&scheduler->runnable)) {
    return list_entry(scheduler->runnable.next, struct sched_task, ll);
  }

  assert(scheduler->current_task);
  return scheduler->current_task;
}

/**
 * Bottom-half of the task switch mechanism. Implemented in pure asm so we don't
 * accidentally clobber registers.
 */
void _sched_task_switch_stack(void *old_stk, void *new_stk);

void sched_task_switch(struct sched_task *task) {
  assert(task);
  struct sched_task *old_task = task->parent->current_task;

  // Switch to self, nothing to do here.
  if (task == old_task) {
    return;
  }

  // Top-half.
  sched_task_switch_nostack(task);

  // Bottom-half.
  _sched_task_switch_stack(&old_task->stk, task->stk);
}

void sched_task_switch_nostack(struct sched_task *task) {
  if (task->state == SCHED_RUNNING) {
    // Nothing to do.
    return;
  }

  assert(task->state != SCHED_BLOCKED);

  struct scheduler *scheduler = task->parent;
  struct sched_task *old_task = scheduler->current_task;

  // Clean up the old task. It can be null during the bootstrap process.
  // TODO(jlam55555): `likely()`.
  if (old_task) {
    list_add(&scheduler->runnable, &old_task->ll);
    if (old_task->state == SCHED_RUNNING) {
      // If the old task was timer-preempted (it wasn't scheduled away due to
      // blocking), then set it to runnable.
      old_task->state = SCHED_RUNNABLE;
    }
  }

  // Set up the new task.
  list_del(&task->ll);
  task->state = SCHED_RUNNING;

  // Adjust scheduler state.
  scheduler->current_task = task;
}
