/**
 * Resources for user-space processes.
 */

#ifndef PROC_PROCESS_H
#define PROC_PROCESS_H

#include "mem/virt.h" // for struct vm_area

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
 * Jump into userspace. This performs an `iret` as described in
 * https://wiki.osdev.org/Getting_to_Ring_3#Entering_Ring_3.
 */
void proc_jump_userspace(void (*cb)(void));

#endif // PROC_PROCESS_H
