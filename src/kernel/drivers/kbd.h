/**
 * The keyboard driver provides a handler for the keyboard IRQ.
 * Usually this means forwarding the request to the terminal
 * driver master side.
 */
#ifndef KBD_H
#define KBD_H

struct kbd_driver {
  void (*kbd_irq)(char);
};

struct kbd_driver *get_default_kbd_driver(void);

#endif // KBD_H
