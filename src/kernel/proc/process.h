/**
 * Structs related to a process.
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

#endif // PROC_PROCESS_H
