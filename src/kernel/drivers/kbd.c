#include "kbd.h"
#include "term.h"

static void default_handle_kbd_irq(char scan_code) {
  struct term_driver *term_driver = get_default_term_driver();
  term_driver->handle_master_write(term_driver, &scan_code, 1);
}

struct kbd_driver default_kbd_driver = {
    .handle_kbd_irq = default_handle_kbd_irq,
};
