#include "kbd.h"
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

struct term_driver *_term_driver;
static void _kbd_irq(uint8_t data) {
  if (data == PS2_KBD_ACK) {
    // TODO(jlam55555): Send next command, if applicable.
    return;
  }
  if (data == PS2_KBD_RESEND) {
    // TODO(jlam55555): Resend last command.
    return;
  }

  _term_driver->master_write(_term_driver->dev, (char *)&data, 1);
}

static void wait() {
  /* volatile int j = 0; */
  /* for (int i = 0; i < 100000000; ++i) { */
  /*   ++j; */
  /* } */
  while (!(inb(0x64) & 1))
    ;
}

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
  outb(0x03, 0x60);
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
