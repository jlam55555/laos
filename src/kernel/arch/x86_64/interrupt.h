/**
 * Interrupts. These data structures
 * (interrupt frames, IDT) are x86_64 specific.
 */
#ifndef ARCH_x86_64_INTERRUPT_H
#define ARCH_x86_64_INTERRUPT_H

#include "idt.h"

// See https://wiki.osdev.org/Interrupt_Service_Routines
struct interrupt_frame {
  size_t ip;
  size_t cs;
  size_t flags;
  size_t sp;
  size_t ss;
};

// Exceptions have an additional error code pushed onto the stack.
struct exception_frame {
  size_t code;
  size_t ip;
  size_t cs;
  size_t flags;
  size_t sp;
  size_t ss;
};

extern struct gate_desc gates[64];
extern struct idtr_desc idtr;

// See https://wiki.osdev.org/8259_PIC
#define PIC1 0x20 /* IO base address for master PIC */
#define PIC2 0xA0 /* IO base address for slave PIC */
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1 + 1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2 + 1)
#define PIC_EOI 0x20 /* End-of-interrupt command code */

void pic_send_eoi(unsigned char irq);

void create_interrupt_gate(struct gate_desc *gate_desc, void *isr);

void init_interrupts(void);

#endif // ARCH_x86_64_INTERRUPT_H
