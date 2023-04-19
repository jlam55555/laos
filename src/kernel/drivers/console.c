#include <stdbool.h>

#include "console.h"

// VGA video buffer memory address.
static volatile char *_video_mem = (volatile char *)0xB8000;

// Video memory.
static const struct console_spec _console_spec = {
    .win_rows = 25,
    .win_cols = 80,
    .scrollback = 100,
};

// Scrollback buffer. This is hardcoded to the console spec;
// we don't need to create multiple of these dynamically and we
// don't have dynamic memory allocation set up.
static char _console_buf[100 * 2 * 80] = {};

static const struct console_cursor _default_console_cursor = {
    .row = 0,
    .col = 0,
    .win_top = 0,
    .color = 0,
};

static struct console _console = {
    .spec = _console_spec,
    .buf = _console_buf,
};

void _init_driver(struct console_driver *driver) {
  driver->console = _console;
  driver->console.cursor = _default_console_cursor;
}

/**
 * Update VGA text mode memory from the scrollback buffer.
 * This should be called each time the scrollback buffer is
 * modified.
 *
 * This copies the whole window to VGA memory, even if
 * only a small part has changed. A more optimal solution
 * may be to allow selective updating of the video memory buffer,
 * but this will be more complicated and error-prone.
 */
static void _console_refresh(struct console *console) {
  int r = console->spec.win_rows;
  int c = console->spec.win_cols;
  int win_top = console->cursor.win_top;
  for (int i = 0; i < r; ++i) {
    for (int j = 0; j < 2 * c; ++j) {
      _video_mem[i * 2 * c + j] = console->buf[(win_top + i) * 2 * c + j];
    }
  }
}

static void _console_advance(struct console *, bool);
static void _console_putchar(struct console *console, char c, bool refresh) {
  // TODO(jlam55555): Implement special handling for newline
  //    and other special characters.

  // Update the scrollback buffer.
  struct console_cursor *cur = &console->cursor;
  int pos = cur->row * 2 * console->spec.win_cols + 2 * cur->col;
  console->buf[pos] = c;
  console->buf[pos + 1] = cur->color;

  _console_advance(console, false);
  if (refresh) {
    _console_refresh(console);
  }
}

static void _console_write(struct console *console, const char *buf,
                           size_t len) {
  for (size_t i = 0; i < len; ++i) {
    _console_putchar(console, buf[i], false);
  }
  _console_refresh(console);
}

static void _console_scroll_abs(struct console *console, int win_top) {
  // Can't scroll back past the beginning of the scrollback buffer.
  if (win_top < 0) {
    console->cursor.win_top = 0;
    return;
  }

  // If we scroll past the scrollback buffer, then
  // copy lines backwards, clear new lines.
  if (win_top > (int)(console->spec.scrollback - console->spec.win_rows)) {
    int new_line_count =
        win_top - (console->spec.scrollback - console->spec.win_rows);
    win_top -= new_line_count;

    // Copy rows.
    char *buf = console->buf;
    for (int i = 0; i < (int)console->spec.scrollback; ++i) {
      int zero = i + new_line_count >= (int)console->spec.scrollback;
      for (int j = 0; j < (int)console->spec.win_cols; ++j) {
        int pos1 = i * 2 * console->spec.win_cols + 2 * j;
        int pos2 = (i + new_line_count) * 2 * console->spec.win_cols + 2 * j;
        buf[pos1] = zero ? ' ' : buf[pos2];
        buf[pos1 + 1] = zero ? 0x0 : buf[pos2 + 1];
      }
    }

    // Move cursor backwards.
    console->cursor.row -= new_line_count;
  }

  // Move window top.
  console->cursor.win_top = win_top;

  _console_refresh(console);
}

static void _console_scroll(struct console *console, int lines) {
  _console_scroll_abs(console, console->cursor.win_top + lines);
}

static void _console_advance(struct console *console, bool refresh) {
  if (++console->cursor.col == console->spec.win_cols) {
    console->cursor.col = 0;

    // Note: if we're at the end of the scrollback buffer (i.e.,
    // if term_cursor.row becomes equal to win_scrollback after
    // the increment, it will temporarily be outside the allowable
    // range of [0, win_scrollback). However, it should be promptly
    // rectified by the call to `term_scroll()`.
    if (++console->cursor.row ==
        console->cursor.win_top + console->spec.win_rows) {
      _console_scroll(console, 1);
    }
  }

  if (refresh) {
    _console_refresh(console);
  }
}

static struct console_driver _console_driver = {
    .init_driver = _init_driver,
    .handle_scroll = _console_scroll,
    .handle_write = _console_write,
};

struct console_driver *get_default_console_driver(void) {
  static bool is_init = false;
  if (!is_init) {
    _console_driver.init_driver(&_console_driver);
    is_init = true;
  }
  return &_console_driver;
}
