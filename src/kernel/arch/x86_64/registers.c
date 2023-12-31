#include "arch/x86_64/registers.h"

struct regs _regs;

void reg_read(struct regs *regs) {
  // Stack pointer will be off by 8, since %rip has already been pushed onto
  // the stack. %rip will already have been pushed on the stack.
  //
  // Note that `push %rsp; ... pop %rsp` should be well-behaved.
  //
  // %rdi will also be wrong, because it's used for the input argument. There
  // are probably ways to get around this (e.g., make this into a macro, make
  // `regs` a global variable, etc.) but I don't think this is too important.
  __asm__ volatile("push %%rsp"
                   "\n\tpushf"
                   "\n\tpush %%r15"
                   "\n\tpush %%r14"
                   "\n\tpush %%r13"
                   "\n\tpush %%r12"
                   "\n\tpush %%r11"
                   "\n\tpush %%r10"
                   "\n\tpush %%r9"
                   "\n\tpush %%r8"
                   "\n\tpush %%rbp"
                   "\n\tpush %%rdi"
                   "\n\tpush %%rsi"
                   "\n\tpush %%rdx"
                   "\n\tpush %%rcx"
                   "\n\tpush %%rbx"
                   "\n\tpush %%rax"

                   // Copy the stack into the output struct.
                   "\n\tmov %0, %%rdi"
                   "\n\tmov %%rsp, %%rsi"
                   "\n\tmov %1, %%rdx"
                   "\n\tcall memcpy"

                   "\n\tpop %%rax"
                   "\n\tpop %%rbx"
                   "\n\tpop %%rcx"
                   "\n\tpop %%rdx"
                   "\n\tpop %%rsi"
                   "\n\tpop %%rdi"
                   "\n\tpop %%rbp"
                   "\n\tpop %%r8"
                   "\n\tpop %%r9"
                   "\n\tpop %%r10"
                   "\n\tpop %%r11"
                   "\n\tpop %%r12"
                   "\n\tpop %%r13"
                   "\n\tpop %%r14"
                   "\n\tpop %%r15"
                   "\n\tpopf"
                   "\n\tpop %%rsp"
                   "\n\tret"
                   :
                   : "r"(regs), "i"(sizeof *regs));
}
