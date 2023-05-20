#include "common/process.h"

#include <stdint.h>

char SAMPLE_STACK[4096];

void *_saved_rsp;

/**
 * Return from stack trampoline.
 *
 * Restore state and perform regular function epilogue.
 *
 * This is probably easier to write as a pure assembly function,
 * but I'm trying to learn how to properly use (extended) asm.
 * The clobber list is a bit conservative; this shouldn't clobber
 * any registers other than the ones explicitly mentioned.
 * Thus, adding the important registers to the clobber list should
 * be redundant.
 *
 * Checking the disassembled code should make it clear that this
 * has the intended effect.
 */
__attribute__((naked)) int _trampoline_stack_ret(void) {
  __asm__ volatile("mov %0, %%rsp\n\t"
                   "pop %%r15\n\t"
                   "pop %%r14\n\t"
                   "pop %%r13\n\t"
                   "pop %%r12\n\t"
                   "pop %%rbp\n\t"
                   "pop %%rbx\n\t"
                   "ret"
                   :
                   : "m"(_saved_rsp)
                   : "rbx", "rsp", "rbp", "r12", "r13", "r14", "r15", "rax");
}

/**
 * Stack trampoline. Save state, create new stack, and call the function.
 *
 * See notes about the extended assembly in _trampoline_stack_ret(), many
 * of the considerations are still true here.
 */
__attribute__((naked)) int trampoline_stack(void *new_stk, int (*fn)(void)) {
  __asm__("push %%rbx\n\t"
          "push %%rbp\n\t"
          "push %%r12\n\t"
          "push %%r13\n\t"
          "push %%r14\n\t"
          "push %%r15\n\t"
          "mov %%rsp, %0\n\t"
          "mov %1, %%rsp\n\t"
          "mov %1, %%rbp\n\t"
          "push %2\n\t"
          "jmp %3"
          : "=m"(_saved_rsp)
          : "r"(new_stk), "e"(&_trampoline_stack_ret), "r"(fn)
          : "rbx", "rsp", "rbp", "r12", "r13", "r14", "r15");
}
