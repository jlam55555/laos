#include "diag/shell.h"

#include "drivers/console.h"
#include "drivers/term.h"

struct term_driver *term_driver;

void shell_init() {
  // Set console properties.
  struct console_driver *console_driver = get_default_console_driver();
  struct console *console = console_driver->dev;
  console->cursor.color = 0x1f;

  // Get default terminal.
  term_driver = get_default_term_driver();
}

void shell_on_interrupt() {
  static char buf[TERM_BUF_SIZE];
  size_t bytes;
  if (!(bytes = term_driver->slave_read(term_driver->dev, buf, sizeof(buf)))) {
    return;
  }

  // Handle input.
  for (size_t i = 0; i < bytes; ++i) {
    if (buf[i] == '\n') {
      term_driver->slave_write(term_driver->dev, "$ ", 2);
    }
  }
}
