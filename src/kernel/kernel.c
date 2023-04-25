#include <limine.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include "arch/x86_64/interrupt.h"
#include "common/util.h"
#include "diag/shell.h"
#include "drivers/kbd.h"

static void _done(void) {
  for (;;) {
    __asm__("hlt");
    shell_on_interrupt();
  }
}

__attribute__((interrupt)) void
_timer_irq(__attribute__((unused)) struct interrupt_frame *frame) {
  // Nothing to do for now.
  pic_send_eoi(0);
}

static struct kbd_driver *_kbd_driver;
__attribute__((interrupt)) void
_kb_irq(__attribute__((unused)) struct interrupt_frame *frame) {
  // Pass scancode to keyboard IRQ handler.
  // TODO(jlam55555): Make the IRQ function accept the interrupt frame
  //    rather than the character.
  _kbd_driver->kbd_irq(inb(0x60));
  pic_send_eoi(1);
}

/**
 * Kernel entry point.
 */
void _start(void) {
  // Initialize keyboard driver. Note that we want to do this
  // before enabling interrupts, due to the nature of the
  // keyboard initialization code.
  _kbd_driver = get_default_kbd_driver();

  // Set up basic timer and keyboard interrupts.
  create_interrupt_gate(&gates[32], _timer_irq);
  create_interrupt_gate(&gates[33], _kb_irq);
  init_interrupts();

  // Simple diagnostic shell.
  shell_init();

  // We're done, just wait for interrupt...
  _done();
}
