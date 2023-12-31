/**
 * Utilities for the user-space process model.
 */

#ifndef PROC_PROCESS_H
#define PROC_PROCESS_H

#include "arch/x86_64/entry.h" // for arch_jump_userspace
#include "mem/virt.h"          // for struct vm_area

/**
 * Analogous to `struct task_struct` in Linux.
 */
struct process {
  struct vm_area *vm;
};

/**
 * Analogous to `current` in Linux.
 */
extern struct process *curr_process;

/**
 * Jump into userspace.
 */
#define proc_jump_userspace arch_jump_userspace

#endif // PROC_PROCESS_H
