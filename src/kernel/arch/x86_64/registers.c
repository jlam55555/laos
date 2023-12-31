#include "arch/x86_64/registers.h"
#include "arch/x86_64/gdt.h"
#include <stdint.h>

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
  //
  // This can (should?) be written as a pure assembly function.
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

void msr_read(uint32_t msr, uint64_t *val) {
  // `rdmsr` reads the MSR into %edx:%ecx.
  __asm__("rdmsr"
          : "=a"(*(uint32_t *)val), "=d"(*((uint32_t *)val + 1))
          : "c"(msr));
}

void msr_write(uint32_t msr, uint64_t value) {
  // `wrmsr` writes the MSR from %edx:%ecx. The high bits of %rax/%rcx are
  // ignored.
  __asm__("wrmsr" : : "c"(msr), "a"(value), "d"(*((uint32_t *)&value + 1)));
}

static void _syscall_enter(void) {
  // Syscall entry point. Just for testing for now.
  // This should be moved out into a different file.
  printf("Entered the kernel again!\r\n");
  for (;;) {
  }
}

void msr_enable_sce() {
  // Set SCE (SysCall Extension) bit. Union is to get around the strict-aliasing
  // rule.
  union {
    struct ia32_efer_msr a;
    uint64_t b;
  } efer;
  msr_read(MSR_IA32_EFER, &efer.b);
  efer.a.sce = 1;
  msr_write(MSR_IA32_EFER, efer.b);

  // Set up syscall target address.
  msr_write(MSR_IA32_LSTAR, (uint64_t)&_syscall_enter);

  // Set up the syscall/sysret segment selectors.
  union {
    struct msr_ia32_star a;
    uint64_t b;
  } star;
  star.b = 0;
  star.a.enter_segment = (struct segment_selector){
      .index = GDT_SEGMENT_RING0_CODE,
      .rpl = 0,
  };
  star.a.exit_segment = (struct segment_selector){
      .index = GDT_SEGMENT_RING3_CODE,
      .rpl = 3,
  };
  msr_write(MSR_IA32_STAR, star.b);

  // Set up the syscall flags. 0 means don't mask any flags.
  msr_write(MSR_IA32_FMASK, 0);
}
