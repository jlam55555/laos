#include "arch/x86_64/init.h"

#include "arch/x86_64/gdt.h"       // for gdt_init
#include "arch/x86_64/interrupt.h" // for idt_init
#include "arch/x86_64/registers.h" // for msr_enable_scr

void arch_init(void) {
  // Set up GDT/IDT/TSS. GDT must be set up first because the TSS and IDT refer
  // to it.
  gdt_init();
  idt_init();

  // Enable and set up syscalls.
  msr_enable_sce();
}
