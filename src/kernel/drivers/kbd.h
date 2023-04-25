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
#ifndef DRIVERS_KBD_H
#define DRIVERS_KBD_H

#include <stdint.h>

#include "common/keycodes.h"

/**
 * I/O port addresses for PS/2 keyboard and keyboard controller.
 */
#define PS2_KBD_PORT 0x60
#define PS2_KBDCTRL_PORT 0x64

/**
 * PS/2 status register flags. The status register
 * is read from PS2_KBD_CONTROLLER_PORT.
 *
 * Currently, only the first two bytes are used
 * when polling data or attempting to write data.
 */
#define PS2_KBDCTRL_STATUS_OUT 0x01
#define PS2_KBDCTRL_STATUS_IN 0x02
#define PS2_KBDCTRL_STATUS_SYSTEM 0x04
#define PS2_KBDCTRL_STATUS_CMD_DATA 0x08
#define PS2_KBDCTRL_STATUS_RESERVED 0x10
#define PS2_KBDCTRL_STATUS_RESERVED2 0x20
#define PS2_KBDCTRL_STATUS_TIMEOUT 0x40
#define PS2_KBDCTRL_STATUS_PARITY 0x80

/**
 * PS/2 keyboard controller configuration byte flags.
 */
#define PS2_KBDCTRL_CONFIG_INTR1 0x01
#define PS2_KBDCTRL_CONFIG_INTR2 0x02
#define PS2_KBDCTRL_CONFIG_SYSTEM 0x04
#define PS2_KBDCTRL_CONFIG_ZERO 0x08
#define PS2_KBDCTRL_CONFIG_CLK1 0x10
#define PS2_KBDCTRL_CONFIG_CLK2 0x20
#define PS2_KBDCTRL_CONFIG_TRANSLATE 0x40
#define PS2_KBDCTRL_CONFIG_ZERO2 0x80

/**
 * Commands to send to PS2_KBDCTRL_PORT.
 */
#define PS2_KBDCTRL_CMD_GET_CONFIG 0x20
#define PS2_KBDCTRL_CMD_SET_CONFIG 0x60

/**
 * Commands to send to PS2_KBD_PORT.
 */
#define PS2_KBD_CMD_SCANCODESET 0xF0

/**
 * Special return codes read from PS2_KBD_PORT.
 */
#define PS2_KBD_ACK 0xFA
#define PS2_KBD_RESEND 0xFE

/**
 * Scan code set ID's. These values assume that bit 6 of
 * the PS/2 controller configuration byte is unset.
 */
#define PS2_KBD_SCANCODESET1 1
#define PS2_KBD_SCANCODESET2 2
#define PS2_KBD_SCANCODESET3 3

struct kbd_driver {
  /**
   * Initializes the keyboard driver. This may perform some
   * PS/2 commands that may trigger interrupts, but these interrupts
   * shouldn't be handled by the kbd_irq handler. Thus, interrupts
   * should be disabled during initialization. (Any I/O in the
   * initialization will utilize polling to ensure that data is ready.)
   */
  void (*driver_init)(struct kbd_driver *);

  /**
   * Keyboard IRQ handler. This receives a scancode byte, and should
   * report keycodes to the input subsystem. (Currently, it sends events
   * directly to the terminal driver in lieu of a full input subsystem.)
   */
  void (*kbd_irq)(uint8_t);
};

struct kbd_driver *get_default_kbd_driver(void);

#endif // DRIVERS_KBD_H
