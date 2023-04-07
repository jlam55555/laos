#include <stdbool.h>

#include "terminal.h"

// VGA video buffer memory address.
static volatile char *_term_video = (volatile char *)0xB8000;

// Video memory.
#define TERM_ROWS 25
#define TERM_COLS 80
#define TERM_SCROLLBACK 100
static struct term_spec _term_spec = {
    .win_rows = TERM_ROWS,
    .win_cols = TERM_COLS,
    .scrollback = TERM_SCROLLBACK,
};

// Scrollback buffer.
static char term_buf[TERM_SCROLLBACK][2 * TERM_COLS] = {};

static struct term_cursor _term_cursor = {
    .row = 0,
    .col = 0,
    .win_top = 0,
    .color = 0,
};

const struct term_cursor *term_get_cursor() { return &_term_cursor; }
const struct term_spec *term_get_spec() { return &_term_spec; }

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
static void _term_refresh(void) {
  for (int i = 0; i < TERM_ROWS; ++i) {
    for (int j = 0; j < 2 * TERM_COLS; ++j) {
      _term_video[i * 2 * TERM_COLS + j] = term_buf[_term_cursor.row + i][j];
    }
  }
}

static void _term_advance(bool);
static void _term_putchar(char c, bool refresh) {
  // TODO(jlam55555): Implement special handling for newline
  //    and other special characters.

  // Update the scrollback buffer.
  term_buf[_term_cursor.row][2 * _term_cursor.col] = c;
  term_buf[_term_cursor.row][2 * _term_cursor.col + 1] = c;

  _term_advance(false);
  if (refresh) {
    _term_refresh();
  }
}

void term_putchar(char c) { _term_putchar(c, true); }

/**
 * Simple terminal writing suggestion from OSDev:
 *
 * void write_string(int colour, const char *string) {
 *   volatile char *video = (volatile char *)0xB8000;
 *   while (*string != 0) {
 *     *video++ = *string++;
 *     *video++ = colour;
 *   }
 * }
 */
void term_write(const char *buf, size_t len) {
  for (size_t i = 0; i < len; ++i) {
    _term_putchar(buf[i], false);
  }
  _term_refresh();
}

void term_writez(const char *buf) {
  while (*buf) {
    _term_putchar(*buf++, false);
  }
  _term_refresh();
}

void term_move(size_t row, size_t col) {
  _term_cursor.row = row;
  _term_cursor.col = col;
}

void term_set_color(char color) { _term_cursor.color = color; }

int term_get_row_rel(void) { return _term_cursor.row - _term_cursor.win_top; }

void term_scroll_abs(int win_top) {
  // Can't scroll back past the beginning of the scrollback buffer.
  if (win_top < 0) {
    _term_cursor.win_top = 0;
    return;
  }

  // TODO(jlam55555): Implement this.
  //     We need to be careful about where to place the cursor
  //     if we advance the scrollback buffer.

  _term_refresh();
}

void term_scroll(int lines) { term_scroll_abs(_term_cursor.win_top + lines); }

static void _term_advance(bool refresh) {
  if (++_term_cursor.col == TERM_COLS) {
    _term_cursor.col = 0;

    // Note: if we're at the end of the scrollback buffer (i.e.,
    // if term_cursor.row becomes equal to win_scrollback after
    // the increment, it will temporarily be outside the allowable
    // range of [0, win_scrollback). However, it should be promptly
    // rectified by the call to `term_scroll()`.
    if (++_term_cursor.row == _term_cursor.win_top + TERM_ROWS) {
      term_scroll(1);
    }
  }

  if (refresh) {
    _term_refresh();
  }
}

void term_advance() { _term_advance(true); }

void term_scroll_to_cursor(void) {
  // See note about default strategy in the header declaration.
}

// Don't pollute macro namespace.
// Use term_get_spec() to get these values from external code.
#undef TERM_ROWS
#undef TERM_COLS
#undef TERM_SCROLLBACK
