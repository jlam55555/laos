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

#endif // COMMON_PROCESS_H
