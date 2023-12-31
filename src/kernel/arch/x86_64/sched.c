#include "arch/x86_64/sched.h"

#include <assert.h>

void arch_stack_init(size_t **stk, void (*cb)(struct sched_task *)) {
  assert(stk);
  assert(cb);

  // Push %rip.
  --*stk;
  **stk = (size_t)cb;

  // Push %rbp, %rbx, %r12, %r13, %r14, %r15 (all garbage).
  // (Really these should be zeroed for security reasons.)
  *stk -= 6;
}
