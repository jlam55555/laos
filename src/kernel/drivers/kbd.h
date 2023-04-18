/**
 * The keyboard driver provides a handler for the keyboard IRQ.
 * Usually this means forwarding the request to the terminal
 * driver master side.
 */
#ifndef KBD_H
#define KBD_H

struct kbd_driver {
  void (*handle_kbd_irq)(char);
};

extern struct kbd_driver default_kbd_driver;

#endif // KBD_H
