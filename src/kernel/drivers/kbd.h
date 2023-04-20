/**
 * The keyboard driver provides a handler for the keyboard IRQ.
 * Usually this means forwarding the request to the terminal
 * driver master side.
 *
 * This driver also includes utilities to convert scancodes into
 * keycodes.
 *
 * See also common/keycodes.h for a more thorough description of
 * scancodes, keycodes, and ASCII characters.
 *
 * TODO(jlam55555): Move these resources (and more info about the
 *     keyboard driver/subsystem) to a docs/ file.
 *
 * Useful resources:
 * - OSDev PS/2 keyboard interface (port 0x60):
 * https://wiki.osdev.org/PS/2_Keyboard
 * - OSDEV PS/2 keyboard controller interface (port 0x64):
 * https://wiki.osdev.org/%228042%22_PS/2_Controller
 * - Another description of the PS/2 spec:
 * http://www-ug.eecg.toronto.edu/msl/nios_devices/datasheets/PS2%20Keyboard%20Protocol.htm
 * - Table of scan code sets 1, 2, 3: https://www.vetra.com/scancodes.html
 * - USB HID keyboard usage codes (used as a keycode standard, since PS/2
 * doesn't define one): https://usb.org/sites/default/files/hut1_3_0.pdf#page=89
 */
#ifndef KBD_H
#define KBD_H

#include <stdint.h>

#include "common/keycodes.h"

struct kbd_driver {
  void (*driver_init)(struct kbd_driver *);
  void (*kbd_irq)(uint8_t);
};

struct kbd_driver *get_default_kbd_driver(void);

// TODO(jlam55555): Add utilities to convert scancodes to keycodes.
//    It doesn't have to be in the public interface though.
// TODO(jlam55555): Improve the PS/2 controller/keyboard setup.

#endif // KBD_H
