#include "common/process.h"

#include <stdint.h>

char SAMPLE_STACK[4096];

struct _saved_state {
  uint64_t rbx;
  uint64_t rsp;
  uint64_t rbp;
  uint64_t r12;
  uint64_t r13;
  uint64_t r14;
  uint64_t r15;
};
struct _saved_state _saved_state;

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
  __asm__ volatile("mov %0, %%rbx\n\t"
                   "mov %1, %%rsp\n\t"
                   "mov %2, %%rbp\n\t"
                   "mov %3, %%r12\n\t"
                   "mov %4, %%r13\n\t"
                   "mov %5, %%r14\n\t"
                   "mov %6, %%r15\n\t"
                   "ret"
                   :
                   : "m"(_saved_state.rbx), "m"(_saved_state.rsp),
                     "m"(_saved_state.rbp), "m"(_saved_state.r12),
                     "m"(_saved_state.r13), "m"(_saved_state.r14),
                     "m"(_saved_state.r15)
                   : "rbx", "rsp", "rbp", "r12", "r13", "r14", "r15", "rax");
}

/**
 * Stack trampoline. Save state, create new stack, and call the function.
 *
 * See notes about the extended assembly in _trampoline_stack_ret(), many
 * of the considerations are still true here.
 */
__attribute__((naked)) int trampoline_stack(void *new_stk, void (*fn)(void)) {
  __asm__("mov %%rbx, %0\n\t"
          "mov %%rsp, %1\n\t"
          "mov %%rbp, %2\n\t"
          "mov %%r12, %3\n\t"
          "mov %%r13, %4\n\t"
          "mov %%r14, %5\n\t"
          "mov %%r15, %6\n\t"
          "mov %7, %%rsp\n\t"
          "mov %7, %%rbp\n\t"
          "push %8\n\t"
          "jmp %9"
          : "=m"(_saved_state.rbx), "=m"(_saved_state.rsp),
            "=m"(_saved_state.rbp), "=m"(_saved_state.r12),
            "=m"(_saved_state.r13), "=m"(_saved_state.r14),
            "=m"(_saved_state.r15)
          : "r"(new_stk), "e"(&_trampoline_stack_ret), "r"(fn)
          : "rbx", "rsp", "rbp", "r12", "r13", "r14", "r15");
}
