/**
 * The console driver renders textual output on the screen.
 * This is similar to the vt (virtual terminal) driver in
 * the Linux kernel.
 *
 * The default console driver provides a simple vertical
 * scrolling capability with a constant scrollback size.
 */
#ifndef CONSOLE_H
#define CONSOLE_H

#include <stddef.h>

/**
 * Console descriptor.
 */
struct console_spec {
  // Size of the console window.
  size_t win_rows;
  size_t win_cols;

  // Total number of lines in the scrollback buffer.
  // Should be >= win_rows.
  size_t scrollback;
};

/**
 * Console cursor. Represents the current context
 * for writing to the terminal.
 */
struct console_cursor {
  // Cursor relative to the scrollback buffer.
  // Note that the row is not relative to the window, and may
  // lie outside of the window; to get the cursor row
  // relative to the window, use the term_get_cursor_rel()
  // helper function.
  // - `row` should lie in [0, scrollback).
  // - `col` should lie in [0, win_cols).
  size_t row;
  size_t col;

  // Top of the terminal window relative to scrollback buffer.
  // This should lie in the range [0, scrollback-win_rows].
  size_t win_top;

  // Color of the character to write.
  char color;
};

/**
 * Console object. Includes the console spec, the cursor,
 * and the scrollback buffer (which should have size
 * 2*rows*cols).
 */
struct console {
  struct console_spec spec;
  struct console_cursor cursor;
  char *buf;
};

/**
 * Console driver.
 */
struct console_driver {
  struct console console;
  void (*init_driver)(struct console_driver *);
  void (*handle_scroll)(struct console *, int);
  void (*handle_write)(struct console *, const char *, size_t);
};

struct console_driver *get_default_console_driver(void);

#endif // CONSOLE_H
