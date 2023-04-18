/**
 * The console driver renders textual output on the screen.
 *
 * The default console driver provides a simple vertical
 * scrolling capability with a constant scrollback size.
 */
#ifndef CONSOLE_H
#define CONSOLE_H

#include <stddef.h>

struct console_driver {
  void (*handle_scroll)(struct console_driver *, int);
  void (*handle_write)(struct console_driver *, char *, size_t);
};

struct console_driver *get_default_console_driver(void);

#endif // CONSOLE_H
