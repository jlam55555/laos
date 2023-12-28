#include "arch/x86_64/interrupt.h"

#include "arch/x86_64/gdt.h"
#include "common/util.h"

struct gate_desc gates[64] = {};
struct idtr_desc idtr = {
    .off = (uint64_t)&gates,
    .sz = sizeof(gates) - 1,
};

void pic_send_eoi(unsigned char irq) {
  if (irq >= 8)
    outb(PIC_EOI, PIC2_CMD);

  outb(PIC_EOI, PIC1_CMD);
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
  gate_desc->gate_type = SSTYPE_INTERRUPT_GATE;

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
  outb(0x11, 0x20); // initialize, pic1_cmd
  outb(0x11, 0xA0); // initialize, pic2_cmd
  outb(0x20, 0x21); // 32 (offset), pic1_data
  outb(40, 0xA1);   // 40 (offset), pic2_data
  outb(0x04, 0x21); // slave PIC at IRQ2, pic1_data
  outb(0x02, 0xA1); // cascade identity, pic2_data
  outb(0x01, 0x21); // 8086 mode, pic1_data
  outb(0x01, 0xA1); // 8086 mode, pic2_data

  outb(0xF8, 0x21); // 0b11111000 mask, pic1_data -- unmask irq 0, 1, 2
  outb(0xEF, 0xA1); // 0b11101111 mask, pic2_data -- unmask irq 12

  load_idtr(&idtr);

  /* __asm__ volatile("int $33"); */
  __asm__ volatile("sti");
}

static uint16_t _pic_get_irq_reg(int ocw3) {
  // OCW3 to PIC CMD to get the register values. PIC2 is chained, and
  // represents IRQs 8-15.  PIC1 is IRQs 0-7, with 2 being the chain.
  outb(ocw3, PIC1_CMD);
  outb(ocw3, PIC2_CMD);
  return (((uint16_t)inb(PIC2_CMD)) << 8) | inb(PIC1_CMD);
}

uint16_t pic_get_irr(void) { return _pic_get_irq_reg(PIC_READ_IRR); }
uint16_t pic_get_isr(void) { return _pic_get_irq_reg(PIC_READ_ISR); }
