/**
 * Interrupts. These data structures (interrupt frames, IDT) are x86_64
 * specific.
 *
 * TODO(jlam55555): Clean this up/merge with idt.h.
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

/**
 * PIC ports. See https://wiki.osdev.org/8259_PIC
 */
#define PIC1_CMD 0x20 // IO base address for master PIC
#define PIC2_CMD 0xA0 // IO base address for slave PIC
#define PIC1_DATA (PIC1_CMD + 1)
#define PIC2_DATA (PIC2_CMD + 1)

/**
 * PIC commands.
 */
#define PIC_EOI 0x20      // End-of-interrupt command code
#define PIC_READ_IRR 0x0A // OCW3 irq ready next CMD read
#define PIC_READ_ISR 0x0B // OCW3 irq service next CMD read

void pic_send_eoi(unsigned char irq);

void create_interrupt_gate(struct gate_desc *gate_desc, void *isr);

void init_interrupts(void);

/**
 * Returns the combined value of the cascaded PICs IRQ Request Register.
 */
uint16_t pic_get_irr(void);

/**
 * Returns the combined value of the cascaded PICs In-Service Register.
 */
uint16_t pic_get_isr(void);

/**
 * Set up basic interrupt table.
 */
void idt_init(void);

// Used by keyboard interrupt.
extern struct kbd_driver *_kbd_driver;

#endif // ARCH_x86_64_INTERRUPT_H
