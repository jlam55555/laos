#include "arch/x86_64/interrupt.h"

#include "common/util.h"

struct gate_desc gates[64] = {};
struct idtr_desc idtr = {
    .off = (uint64_t)&gates,
    .sz = sizeof(gates) - 1,
};

void pic_send_eoi(unsigned char irq) {
  if (irq >= 8)
    outb(PIC_EOI, PIC2_COMMAND);

  outb(PIC_EOI, PIC1_COMMAND);
}

void create_interrupt_gate(struct gate_desc *gate_desc, void *isr) {
  // Select 64-bit code segment of the GDT. See
  // https://github.com/limine-bootloader/limine/blob/trunk/PROTOCOL.md#x86_64.
  gate_desc->segment_selector = (struct segment_selector){
      .rpl = 0,
      .ti = 0,
      .index = 5,
  };

  // Don't use IST.
  gate_desc->ist = 0;

  // ISR.
  gate_desc->gate_type = 0xe;

  // Run in ring 0.
  gate_desc->dpl = 0;

  // Present bit.
  gate_desc->p = 1;

  // Set offsets slices.
  gate_desc->off_1 = (size_t)isr;
  gate_desc->off_2 = ((size_t)isr >> 16) & 0xffff;
  gate_desc->off_3 = (size_t)isr >> 32;
}

void init_interrupts(void) {
  // https://forum.osdev.org/viewtopic.php?p=316295#p316295
  outb(0x11, 0x20);
  outb(0x11, 0xA0);
  outb(0x20, 0x21);
  outb(40, 0xA1);
  outb(0x04, 0x21);
  outb(0x02, 0xA1);
  outb(0x01, 0x21);
  outb(0x01, 0xA1);
  outb(0xF8, 0x21);
  outb(0xEF, 0xA1);

  load_idtr(&idtr);

  /* __asm__ volatile("int $33"); */
  __asm__ volatile("sti");
}
