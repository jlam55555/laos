#include "kbd.h"
#include "term.h"

static void _kbd_irq(char scan_code) {
  struct term_driver *term_driver = get_default_term_driver();
  term_driver->master_write(term_driver->dev, &scan_code, 1);
}

static struct kbd_driver _kbd_driver = {
    .kbd_irq = _kbd_irq,
};

struct kbd_driver *get_default_kbd_driver(void) {
  return &_kbd_driver;
}
