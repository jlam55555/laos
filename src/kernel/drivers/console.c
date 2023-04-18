#include "console.h"

static struct console_driver default_console_driver = {};

struct console_driver *get_default_console_driver(void) {
  return &default_console_driver;
}
