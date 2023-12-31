#include "proc/process.h"
#include "arch/x86_64/gdt.h"

struct process *curr_process = NULL;

__attribute__((naked)) void proc_jump_userspace(void (*cb)(void)) {
  static const struct segment_selector ring3_data_selector = {
      .index = GDT_SEGMENT_RING3_DATA,
      .rpl = 3,
  };
  static const struct segment_selector ring3_code_selector = {
      .index = GDT_SEGMENT_RING3_CODE,
      .rpl = 3,
  };

  // TODO(jlam55555): We'll want to set rsp0 to point to the top of the kernel
  // stack. If there are any local variables this may not be the top of the
  // kernel stack. We should get the top of the kernel stack from the sched
  // struct. I'm jus tlazy for now.
  static void *rsp0;
  __asm__("mov %%rsp, %0" : "=m"(rsp0));
  tss_set_kernel_stack(rsp0);

  // Set up the stack. Refer to Intel SDM Vol. 3A Sec. 6.12 Figure 6-4.
  // SS is handled automatically by iret.
  //
  // We're not really supposed to clobber %rsp in extended asm, but this is okay
  // since there's nothing afterwards.
  __asm__ volatile("mov %0, %%ax"
                   "\n\tmov %%ax, %%ds"
                   "\n\tmov %%ax, %%es"
                   "\n\tmov %%ax, %%fs"
                   "\n\tmov %%ax, %%gs"

                   "\n\tmov %%rsp, %%rax"
                   "\n\tpush %1"    // SS
                   "\n\tpush %%rax" // RSP
                   "\n\tpushf"      // RFLAGS
                   "\n\tpush %2"    // CS
                   "\n\tpush %3"    // RIP
                   "\n\tiretq"
                   :
                   // `ring3_*_selector` are already in memory, and `cb` is in a
                   // register. It's important that we don't read `cb` from
                   // memory since that means it'll be put on the stack and we
                   // clobber the stack. It's probably safest to do this from a
                   // pure asm function but this _should_ be fine for now.
                   : "m"(ring3_data_selector), "m"(ring3_data_selector),
                     "m"(ring3_code_selector), "r"(cb));
}
