/**
 * The keyboard driver provides a handler for the keyboard IRQ.
 * Usually this means forwarding the request to the terminal
 * driver master side.
 *
 * Useful resources:
 * - https://wiki.osdev.org/PS/2_Keyboard
 * - https://wiki.osdev.org/%228042%22_PS/2_Controller
 * -
 * http://www-ug.eecg.toronto.edu/msl/nios_devices/datasheets/PS2%20Keyboard%20Protocol.htm
 * - https://www.vetra.com/scancodes.html
 */
#ifndef KBD_H
#define KBD_H

#include <stdint.h>

struct kbd_driver {
  void (*driver_init)(struct kbd_driver *);
  void (*kbd_irq)(uint8_t);
};

struct kbd_driver *get_default_kbd_driver(void);

#endif // KBD_H
