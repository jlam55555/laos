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
 * Trampoline to a new stack.
 *
 * Note: naked function because we want to disable the
 * normal function epilogue.
 */
__attribute__((naked)) int trampoline_stack(void *new_stk, int (*)(void));

#endif // COMMON_PROCESS_H
