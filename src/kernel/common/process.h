/**
 * Support for processes.
 */
#ifndef COMMON_PROCESS_H
#define COMMON_PROCESS_H

/**
 * Sample stack for use.
 */
extern char SAMPLE_STACK[4096];

/**
 * Trampoline to a new stack. This necessarily is
 * implemented in assembly because we want to modify
 * the function prologue/epilogue.
 */
int trampoline_stack(void *new_stk, int (*)(void));

/**
 * Process control block. Represents a process.
 */
struct process {
  void *rsp;
  unsigned pid;
  enum process_status {
    PROCESS_RUNNING,
    PROCESS_SLEEPING,
  } status;
};

/**
 * The following functions should only be called from schedulable
 * (process) context. To jump into schedulable (process) context,
 * use the trampoline function above.
 */

/**
 * Get the current PID, if in process context. Otherwise, return -1.
 */
int get_current_pid(void);

/**
 * Choose a process to schedule. If `pid` is provided (pid!=-1),
 * then we will try to schedule that pid, if it exists.
 */
int schedule(int pid);

/**
 * Called by a process when it wants to exit. If this is the
 * root process (pid=0), then this will return to non-process context.
 * Otherwise, it will result in a call to schedule().
 *
 * This process is set as the base pointer for all processes.
 */
void exit(int);

/**
 * Duplicate the running process.
 */
int fork(void);

#endif // COMMON_PROCESS_H
