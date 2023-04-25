#include "drivers/kbd.h"

#include "common/keycodes.h"
#include "common/util.h"
#include "drivers/console.h"
#include "drivers/term.h"

/**
 * Mapping from scancodes to keycodes for scancode set 1.
 * The sign of the value indicates whether it is a make or
 * break code; positive values indicate make codes, and
 * negative values indicate break codes. The value KC_INVAL
 * (0x00) indicates a scancode that doesn't map to a keycode.
 *
 * Notes for scan set 1:
 * - For single-byte codes, the keypress scancodes lie
 *   between 0x01 and 0x58, and the keyup scancodes lie
 *   between 0x81 and 0xD8 (shifted up by 0x80).
 * - There are large gaps at 0x59-0x80 and 0xD9-0xFF, presumably
 *   because there are special PS/2 keyboard bytes in that range
 *   (e.g., 0xE0, 0xFA, etc.)
 *
 * TODO(jlam55555): For now, we assume single-byte scancodes.
 *    We need to handle multi-byte scancodes soon.
 *    Ignore print screen and pause for now; these will be
 *    Handled separately.
 *    This is for scan code set 1 only.
 */
static const int16_t _sc_to_kc_map[256] = {
    /* 0x00 */
    KC_INVAL, KC_ESC, KC_1, KC_2, KC_3, KC_4, KC_5, KC_6, KC_7, KC_8, KC_9,
    KC_0, KC_HYPHEN, KC_EQUALS, KC_BKSP, KC_TAB,
    /* 0x10 */
    KC_Q, KC_W, KC_E, KC_R, KC_T, KC_Y, KC_U, KC_I, KC_O, KC_P, KC_LBRKT,
    KC_RBRKT, KC_RET, KC_LCTRL, KC_A, KC_S,
    /* 0x20 */
    KC_D, KC_F, KC_G, KC_H, KC_J, KC_K, KC_L, KC_SEMICOLON, KC_QUOTE,
    KC_BACKTICK, KC_LSHFT, KC_BKSLASH, KC_Z, KC_X, KC_C, KC_V,
    /* 0x30 */
    KC_B, KC_N, KC_M, KC_COMMA, KC_PERIOD, KC_SLASH, KC_RSHFT, KC_KP_MULTIPLY,
    KC_LALT, KC_SPACE, KC_CAPS_LOCK, KC_F1, KC_F2, KC_F3, KC_F4, KC_F5,
    /* 0x40 */
    KC_F6, KC_F7, KC_F8, KC_F9, KC_F10, KC_NUM_LOCK, KC_SCROLL_LOCK, KC_KP_7,
    KC_KP_8, KC_KP_9, KC_KP_MINUS, KC_KP_4, KC_KP_5, KC_KP_6, KC_KP_ADD,
    KC_KP_1,
    /* 0x50 */
    KC_KP_2, KC_KP_3, KC_KP_0, KC_KP_PERIOD, KC_INVAL, KC_INVAL, KC_INVAL,
    KC_F11, KC_F12, KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL,
    KC_INVAL,
    /* 0x60 */
    KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL,
    KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL,
    KC_INVAL, KC_INVAL,
    /* 0x70 */
    KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL,
    KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL,
    KC_INVAL, KC_INVAL,
    /* 0x80 */
    KC_INVAL, -KC_ESC, -KC_1, -KC_2, -KC_3, -KC_4, -KC_5, -KC_6, -KC_7, -KC_8,
    -KC_9, -KC_0, -KC_HYPHEN, -KC_EQUALS, -KC_BKSP, -KC_TAB,
    /* 0x90 */
    -KC_Q, -KC_W, -KC_E, -KC_R, -KC_T, -KC_Y, -KC_U, -KC_I, -KC_O, -KC_P,
    -KC_LBRKT, -KC_RBRKT, -KC_RET, -KC_LCTRL, -KC_A, -KC_S,
    /* 0xA0 */
    -KC_D, -KC_F, -KC_G, -KC_H, -KC_J, -KC_K, -KC_L, -KC_SEMICOLON, -KC_QUOTE,
    -KC_BACKTICK, -KC_LSHFT, -KC_BKSLASH, -KC_Z, -KC_X, -KC_C, -KC_V,
    /* 0xB0 */
    -KC_B, -KC_N, -KC_M, -KC_COMMA, -KC_PERIOD, -KC_SLASH, -KC_RSHFT,
    -KC_KP_MULTIPLY, -KC_LALT, -KC_SPACE, -KC_CAPS_LOCK, -KC_F1, -KC_F2, -KC_F3,
    -KC_F4, -KC_F5,
    /* 0xC0 */
    -KC_F6, -KC_F7, -KC_F8, -KC_F9, -KC_F10, -KC_NUM_LOCK, -KC_SCROLL_LOCK,
    -KC_KP_7, -KC_KP_8, -KC_KP_9, -KC_KP_MINUS, -KC_KP_4, -KC_KP_5, -KC_KP_6,
    -KC_KP_ADD, -KC_KP_1,
    /* 0xD0 */
    -KC_KP_2, -KC_KP_3, -KC_KP_0, -KC_KP_PERIOD, KC_INVAL, KC_INVAL, KC_INVAL,
    -KC_F11, -KC_F12, KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL,
    KC_INVAL, KC_INVAL,
    /* 0xE0 */
    KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL,
    KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL,
    KC_INVAL, KC_INVAL,
    /* 0xF0 */
    KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL,
    KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL, KC_INVAL,
    KC_INVAL, KC_INVAL};

static bool _pressed_kc_map[256] = {};

/**
 * Converts a keycode and a make/break boolean to a keyboard event.
 *
 * Also updates the _pressed_kc_map mapping. Handles regular keys
 * differently than toggle keys (Caps Lock, Scroll Lock, Num Lock).
 */
static struct kbd_event _generate_kbd_evt(enum keycode kc, bool is_make) {
  enum kbd_event_type type =
      is_make ? (_pressed_kc_map[type] ? KBD_EVENT_KEYUP : KBD_EVENT_KEYDOWN)
              : KBD_EVENT_KEYUP;
  // Non-toggle key.
  if (kc != KC_CAPS_LOCK && kc != KC_SCROLL_LOCK && kc != KC_NUM_LOCK) {
    _pressed_kc_map[kc] = is_make;
  }
  // Caps/Scroll/Num-lock toggle key (on keydown event).
  // TODO(jlam55555): Note that we may need an override if any of these keys
  //    are mapped to other keys. For example, in Colemak the Caps Lock key
  //    is mapped to backspace, which means that we don't want a toggle
  //    behavior. This is relatively niche though, and could also be solved
  //    in userspace. The easiest way to solve this may be to have a list of
  //    toggle/lock keys for each keyboard layout, along with which bit they
  //    toggle in the keyboard modifiers byte (out of bytes 3-7).
  else {
    _pressed_kc_map[kc] =
        type == KBD_EVENT_KEYDOWN ? !_pressed_kc_map[kc] : _pressed_kc_map[kc];
  }
  uint8_t modifiers =
      ((_pressed_kc_map[KC_LCTRL] || _pressed_kc_map[KC_RCTRL]) ? KM_CTRL : 0) |
      ((_pressed_kc_map[KC_LSHFT] || _pressed_kc_map[KC_RSHFT]) ? KM_SHFT : 0) |
      ((_pressed_kc_map[KC_LALT] || _pressed_kc_map[KC_RALT]) ? KM_ALT : 0) |
      (_pressed_kc_map[KC_CAPS_LOCK] ? KM_CAPS_LOCK : 0) |
      (_pressed_kc_map[KC_SCROLL_LOCK] ? KM_SCROLL_LOCK : 0) |
      (_pressed_kc_map[KC_NUM_LOCK] ? KM_NUM_LOCK : 0);
  struct kbd_event evt = {
      .kc = kc,
      .km = modifiers,
      .type = type,
  };
  kc_to_ascii(&evt, kc_to_ascii_map_qwerty);
  return evt;
}

/**
 * Customizable keyboard event handler, called by the IRQ.
 *
 * TODO(jlam55555): For now not really customizable. Right now
 *    the only "customization" is for the terminal. Later on this
 *    may be generalized as part of a broader input subsystem.
 * TODO(jlam55555): Convert special characters, such as arrow keys,
 *    into multi-byte ANSI escape sequences. This should only be done
 *    for the terminal, and not other parts of the input subsystem.
 */
struct term_driver *_term_driver;
static void _handle_evt(struct kbd_event evt) {
  // evt.ascii < 0 indicates a key that doesn't correspond to an ASCII
  // character. We would still report this to a more generalized input
  // subsystem that cares about all keyboard events, but no
  // data would be sent to the terminal. Similarly, the terminal
  // only cares about keydown/keypress events.
  if (evt.ascii < 0 || evt.type == KBD_EVENT_KEYUP) {
    return;
  }

  // Alt-key handling: insert an escape byte beforehand.
  // This can happen in conjunction with control sequences.
  if (evt.km & KM_ALT) {
    _term_driver->master_write(_term_driver->dev, "\e", 1);
  }

  // Ctrl-key handling: send control sequence instead of ASCII character.
  if (evt.km & KM_CTRL && evt.ascii >= 0x40) {
    char c = evt.ascii & ~0x60;
    _term_driver->master_write(_term_driver->dev, &c, 1);
  }
  // Non-control key.
  else {
    _term_driver->master_write(_term_driver->dev, &evt.ascii, 1);
  }
}

static void _kbd_irq(uint8_t data) {
  // During normal operation, we don't expect to be sending any commands
  // that would return an ACK or RESEND, so ignore these for now. If we
  // decide to send commands, then we should account for these.
  //
  // The only time commands are being sent at the time of writing are
  // during initialization, which should not be raising interrupts and
  // thus will not be handled here.
  if (data == PS2_KBD_ACK || data == PS2_KBD_RESEND) {
    return;
  }

  // TODO(jlam55555): Keep reading until the end of the scancode sequence.
  // TODO(jlam55555): Handle multi-byte scancodes.
  int16_t kc_make_break = _sc_to_kc_map[data];
  bool is_make = kc_make_break >= 0;
  enum keycode kc = is_make ? kc_make_break : -kc_make_break;

  if (kc == KC_INVAL) {
    // TODO(jlam55555): Some error handling here.
    return;
  }

  struct kbd_event evt = _generate_kbd_evt(kc, is_make);

  // Similar to input_report_key() in Linux.
  _handle_evt(evt);
}

/**
 * Waits for the output buffer to be ready. This should return
 * true before attempting to read from PS2_KBD_PORT.
 */
static void _ps2_kbd_wait_for_output(void) {
  while (!(inb(PS2_KBDCTRL_PORT) & PS2_KBDCTRL_STATUS_OUT)) {
  }
}

/**
 * Waits for the input buffer to be ready. This should return
 * true before attempting to write to PS2_KBD_PORT or PS2_KBDCTRL_PORT.
 */
static void _ps2_kbd_wait_for_input(void) {
  while (inb(PS2_KBDCTRL_PORT) & PS2_KBDCTRL_STATUS_IN) {
  }
}

/**
 * Read keyboard controller configuration byte.
 */
static uint8_t _ps2_kbdctrl_get_config(void) {
  _ps2_kbd_wait_for_input();
  outb(PS2_KBDCTRL_CMD_GET_CONFIG, PS2_KBDCTRL_PORT);
  _ps2_kbd_wait_for_output();
  return inb(PS2_KBD_PORT);
}

/**
 * Set keyboard controller configuration byte.
 */
static void _ps2_kbdctrl_set_config(uint8_t config) {
  _ps2_kbd_wait_for_input();
  outb(PS2_KBDCTRL_CMD_SET_CONFIG, PS2_KBDCTRL_PORT);
  _ps2_kbd_wait_for_input();
  outb(config, PS2_KBD_PORT);
}

/**
 * Read scancode set. Note that the output of this
 * function depends on the value of bit 6 of the
 * PS/2 controller configuration byte (scancode set
 * 2->1 translation). If that bit is unset, then this
 * should return 1, 2, or 3 for scancode sets 1, 2,
 * or 3, respectively. If that bit is set, then this
 * will return 0x43, 0x41, or 0x3f, respectively.
 *
 * TODO(jlam55555): Error checking on return values.
 */
static uint8_t _ps2_kbd_get_scancodeset(void) {
  _ps2_kbd_wait_for_input();
  outb(PS2_KBD_CMD_SCANCODESET, PS2_KBD_PORT);
  _ps2_kbd_wait_for_output();
  inb(PS2_KBD_PORT);
  _ps2_kbd_wait_for_input();
  // 0x00 is the read scancodeset subcommand.
  outb(0x00, PS2_KBD_PORT);
  _ps2_kbd_wait_for_output();
  inb(PS2_KBD_PORT);
  _ps2_kbd_wait_for_output();
  return inb(PS2_KBD_PORT);
}

/**
 * Set scancode set.
 */
static void _ps2_kbd_set_scancodeset(uint8_t scancodeset) {
  outb(PS2_KBD_CMD_SCANCODESET, PS2_KBD_PORT);
  _ps2_kbd_wait_for_output();
  inb(PS2_KBD_PORT);
  _ps2_kbd_wait_for_input();
  outb(scancodeset, PS2_KBD_PORT);
  _ps2_kbd_wait_for_output();
  return inb(PS2_KBD_PORT);
}

static void _driver_init(__attribute__((unused)) struct kbd_driver *driver) {
  // Consume outstanding output from device.
  while (inb(PS2_KBDCTRL_PORT) & PS2_KBDCTRL_STATUS_OUT) {
    inb(PS2_KBD_PORT);
  }

  // Update configuration. Turn off scancode set 2->1 translation.
  // This makes the behavior less confusing IMO.
  _ps2_kbdctrl_set_config(_ps2_kbdctrl_get_config() &
                          ~PS2_KBDCTRL_CONFIG_TRANSLATE);

  // Set scan code set 1, if not set already.
  // (Default is scan code set 2).
  if (_ps2_kbd_get_scancodeset() != PS2_KBD_SCANCODESET1) {
    _ps2_kbd_set_scancodeset(PS2_KBD_SCANCODESET1);
  }

  // Activate device and enable interrupts.
  // TODO(jlam55555): Do this properly, following the OSDev wiki.
  //    The above by itself works on QEMU, but this is not robust by
  //    any means.

  // Set term_driver, for use in the IRQ handler. Later, this should
  // be moved to the input subsystem when that's fleshed out more.
  _term_driver = get_default_term_driver();
}

static struct kbd_driver _kbd_driver = {
    .driver_init = _driver_init,
    .kbd_irq = _kbd_irq,
};

struct kbd_driver *get_default_kbd_driver(void) {
  static bool is_init = false;
  if (!is_init) {
    _kbd_driver.driver_init(&_kbd_driver);
  }
  return &_kbd_driver;
}
