#include "arch/x86_64/interrupt.h"

#include "arch/x86_64/gdt.h"
#include "common/util.h"
#include "drivers/kbd.h"
#include "sched/sched.h" // for schedule

// TODO(jlam55555): We shouldn't really be printf()-ing in interrupts.
#include "common/libc.h" // for printf

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
      .index = GDT_SEGMENT_RING0_CODE,
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

static __attribute__((interrupt)) void
_timer_irq(__attribute__((unused)) struct interrupt_frame *frame) {
  // Pre-emptive scheduling.
  pic_send_eoi(/*interrupt_num=*/0);
  schedule();
}

struct kbd_driver *_kbd_driver;
static __attribute__((interrupt)) void
_kb_irq(__attribute__((unused)) struct interrupt_frame *frame) {
  // Pass scancode to keyboard IRQ handler.
  // TODO(jlam55555): Make the IRQ function accept the interrupt frame
  //    rather than the character.
  _kbd_driver->kbd_irq(inb(0x60));
  pic_send_eoi(/*interrupt_num=*/1);
}

static __attribute__((interrupt)) void
_gp_isr(__attribute((unused)) struct exception_frame *frame) {
  printf("gp fault\r\n");
  // Infinite loop so QEMU doesn't crash and we can debug the stack frame.
  for (;;) {
  }
}

static __attribute__((interrupt)) void
_pf_isr(__attribute((unused)) struct exception_frame *frame) {
  printf("page fault\r\n");
  for (;;) {
  }
}

static __attribute__((interrupt)) void
_div_isr(__attribute((unused)) struct interrupt_frame *frame) {
  printf("div zero\r\n");
  for (;;) {
  }
}

static __attribute__((interrupt)) void
_ud_isr(__attribute((unused)) struct interrupt_frame *frame) {
  printf("invalid opcode\r\n");
  for (;;) {
  }
}

void idt_init(void) {
  create_interrupt_gate(&gates[0], _div_isr);
  create_interrupt_gate(&gates[6], _ud_isr);
  create_interrupt_gate(&gates[13], _gp_isr);
  create_interrupt_gate(&gates[14], _pf_isr);
  create_interrupt_gate(&gates[32], _timer_irq);
  create_interrupt_gate(&gates[33], _kb_irq);

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
