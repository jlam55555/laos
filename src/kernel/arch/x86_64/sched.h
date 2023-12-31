/**
 * Architecture-specific stack layout. These are to be used by the arch-agnostic
 * thread scheduling code in sched/sched.c.
 */
#ifndef ARCH_X86_64_SCHED_H
#define ARCH_X86_64_SCHED_H

#include "sched/sched.h" // for struct sched_task

/**
 * Initialize the stack, so it look like we're in the middle of the
 * `sched_task_switch()` function. This modifies the stack pointer.
 */
void arch_stack_init(size_t **stk, void (*cb)(struct sched_task *));

/**
 * Switch stacks. The new and old stack should look like they're in the middle
 * of the `sched_task_switch()` function. Implemented in pure asm so we don't
 * accidentally clobber registers.
 */
void arch_stack_switch(void **old_stk, void *new_stk);

/**
 * Very simple helper function used in the VMM initialization. Jump to a new
 * stack and %rip (doesn't assume the stacks look like they're in the middle of
 * the `sched_task_switch()` function).
 */
__attribute__((noreturn)) void arch_stack_jmp(void *stk, void *rip);

#endif // ARCH_X86_64_SCHED_H
