#include "kbd.h"
#include "common/keycodes.h"
#include "console.h"
#include "term.h"
#include "util.h"

// Port address for PS/2 keyboard.
#define PS2_KBD_PORT 0x64
#define PS2_KBD_GET_SETSCANCODESET 0xF0
#define PS2_KBD_GET_SETSCANCODESET_GET 0x01
#define PS2_KBD_GET_SETSCANCODESET_SET1 0x02
#define PS2_KBD_GET_SETSCANCODESET_SET2 0x03
#define PS2_KBD_GET_SETSCANCODESET_SET3 0x04

#define PS2_KBD_ACK 0xFA
#define PS2_KBD_RESEND 0xFE

// TODO(jlam55555): For now, we assume single-byte scancodes.
//    We need to handle multi-byte scancodes soon.
//    Ignore print screen and pause for now; these will be
//    Handled separately.
//    This is for scan code set 1 only.
// Notes for scan set 1:
// - For single-byte codes, the keypress scancodes lie
//   between 0x01 and 0x58, and the keyup scancodes lie
//   between 0x81 and 0xD8 (shifted up by 0x80).
// - There are large gaps at 0x59-0x80 and 0xD9-0xFF, presumably
//   because there are special PS/2 keyboard bytes in that range
//   (e.g., 0xE0, 0xFA, etc.)
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
 *    into multi-byte ANSI escape sequences.
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
  if (data == PS2_KBD_ACK) {
    // TODO(jlam55555): Send next command, if applicable.
    return;
  }
  if (data == PS2_KBD_RESEND) {
    // TODO(jlam55555): Resend last command.
    return;
  }

  // TODO(jlam55555): Keep reading until the end of the scancode sequence.
  // TODO(jlam55555): Handle multi-byte scancodes.
  // TODO(jlam55555): Handle keys that toggle state, e.g., Caps Lock.
  int16_t kc_make_break = _sc_to_kc_map[data];
  bool is_make = kc_make_break >= 0;
  enum keycode kc = is_make ? kc_make_break : -kc_make_break;

  if (kc == KC_INVAL) {
    // TODO(jlam55555): Some error handling here.
    return;
  }

  struct kbd_event evt = _generate_kbd_evt(kc, is_make);

  _handle_evt(evt);
}

// Waits for input buffer to be ready.
static void wait() {
  /* volatile int j = 0; */
  /* for (int i = 0; i < 100000000; ++i) { */
  /*   ++j; */
  /* } */
  while (!(inb(0x64) & 1))
    ;
}

// Waits for output buffer to be ready.
static void wait2() {
  /* volatile int j = 0; */
  /* for (int i = 0; i < 100000000; ++i) { */
  /*   ++j; */
  /* } */
  while (inb(0x64) & 0x2)
    ;
}

static void printf(char *c) {
  char *c2;
  for (c2 = c; *c2; ++c2) {
  }
  int strlen = c2 - c;
  struct console_driver *console_driver = get_default_console_driver();
  console_driver->dev->cursor.color = 0x2f;
  console_driver->write(console_driver->dev, c, strlen);
}

static void print(uint8_t c) {
  char buf[10] = {};
  buf[0] = '0';
  buf[1] = 'x';
  buf[2] = "0123456789ABCDEF"[(c & 0xf0) >> 4];
  buf[3] = "0123456789ABCDEF"[c & 0xf];
  buf[4] = ' ';
  printf(buf);
}

static void _driver_init(__attribute__((unused)) struct kbd_driver *driver) {
  // Set up PS/2 scanset.
  /* outb(PS2_KBD_GET_SETSCANCODESET, PS2_KBD_PORT); */
  /* outb(PS2_KBD_GET_SETSCANCODESET_GET, PS2_KBD_PORT); */

  /* printf("Got here 1"); */

  char c;

  // TODO: worry about printing this later.

  while (inb(0x64) & 1) {
    inb(0x60);
  }

  /* outb(0xAE, 0x64); */
  /* wait(); */
  /* print(inb(0x60)); */

  // Get current PS/2 controller configuration byte.
  uint8_t config;
  outb(0x20, 0x64);
  wait();
  print(config = inb(0x60));

  // Set configuration.
  outb(0x60, 0x64);
  wait2();
  outb(config & ~0x40, 0x60);
  wait2();

  /* 0x61 = 01100001 */
  /*   bytes 0, 5, 6 */

  // Get current scan code set.
  outb(0xF0, 0x60);
  wait();
  print(inb(0x60));
  outb(0x00, 0x60);
  wait();
  print(inb(0x60));
  wait();
  print(inb(0x60));

  // Set scan code set.
  outb(0xF0, 0x60);
  wait();
  print(inb(0x60));
  outb(0x01, 0x60);
  wait();
  print(inb(0x60));

  // Get updated scan code set.
  outb(0xF0, 0x60);
  wait();
  print(inb(0x60));
  outb(0x00, 0x60);
  wait();
  print(inb(0x60));
  wait();
  print(inb(0x60));

  /* wait(); */
  /* print(inb(0x60)); */

  /* // Flush keyboard input. */
  /* while (inb(0x64) & 1) { */
  /*   inb(0x60); */
  /* } */

  /* printf("Got here 2"); */

  /* // Activate keyboard device. */
  /* outb(0xAE, 0x64); */
  /* wait(); */

  /* printf("Got here 3"); */

  /* // get status */
  /* outb(0x20, 0x64); */
  /* printf("Got here 4"); */
  /* while (!(inb(0x64) & 1)) { */
  /* } */
  /* /\* wait(); *\/ */
  /* inb(0x60); */
  /* uint8_t status = (inb(0x60) | 1) & 0x05; */
  /* print("Setting PS/2 controller status: 0x"); */
  /* printx(status); */
  /* putc('\n'); */
  /* wait(); */

  /* // set status */
  /* outb(0x60, 0x64); */
  /* wait(); */
  /* outb(status, 0x60); */
  /* wait(); */

  /* // enable keyboard scanning */
  /* outb(0xF4, 0x60); */

  /* inb(0x60); */
  /* outb(0xF0, 0x60); */

  /* outb(0xD1, 0x64); */
  /* inb(0x64); */

  /* outb(0xF0, 0x60); */
  /* for (long i = 0; i < 100000000l; ++i) { */
  /*   ++j; */
  /* } */
  /* inb(0x60); */

  /* outb(0xD1, 0x64); */
  /* for (long i = 0; i < 100000000l; ++i) { */
  /*   ++j; */
  /* } */
  /* inb(0x64); */

  /* outb(0x01, 0x60); */
  /* for (long i = 0; i < 100000000l; ++i) { */
  /*   ++j; */
  /* } */
  /* inb(0x60); */

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
