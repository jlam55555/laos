/**
 * Very simple termnal driver.
 *
 * Limine also provides a struct limine_terminal in
 * common/drivers/vga_textmode.h, but it only exposes a very
 * simple textmode interface. Implementing a textmode terminal
 * is useful for extending this functionality (e.g., positioning
 * text arbitrarily, scrolling) and for educational purposes.
 *
 * Horizontal scrolling is not implemented, because I am lazy
 * and don't think it would give much benefit. It shouldn't be
 * much different from vertical scrolling, however.
 *
 * See: https://wiki.osdev.org/Printing_To_Screen and the
 * Limine terminal driver for more details.
 *
 * TODO(jlam55555): Implement some (fixed-size) scrollback.
 *     Arbitrary scrollback can come once we figure out dynamic
 *     memory management.
 *
 * TODO(jlam55555): Implement blinking cursor.
 */
#ifndef TERMINAL_H
#define TERMINAL_H

#include <stddef.h>

/**
 * Terminal device descriptor. Can retrieve this using
 * term_get_spec().
 */
struct term_spec {
  // Size of the terminal window.
  size_t win_rows;
  size_t win_cols;

  // Total number of lines in the scrollback buffer.
  // Should be >= win_rows.
  size_t scrollback;
};

/**
 * Represents the current context for writing to the
 * terminal. There is a global term_cursor, although
 * it may be possible to store/restore a term_cursor
 * object.
 *
 * Currently, all term_*() functions operate on a global
 * terminal cursor/context, but it may be useful to
 * pass an explicit term_cursor.
 */
struct term_cursor {
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

const struct term_cursor *term_get_cursor(void);
const struct term_spec *term_get_spec(void);
void term_putchar(char);
void term_write(const char *buf, size_t len);
void term_writez(const char *buf);
void term_move(size_t row, size_t col);
void term_set_color(char);
int term_get_row_rel(void);

/**
 * Scroll the window to a given value of win_top.
 * Alternative interface to `term_scroll()`.
 *
 * If `win_top` is within [0, scrollback-win_rows],
 * then the scrolling will occur within the existing
 * scrollback buffer and the cursor will not move.
 *
 * If `win_top` > scroll_back-win_rows, then the scrollback
 * buffer will scroll and the cursor will move.
 *
 * If `win_top` < 0, scroll to the top of the scrollback
 * buffer (set `win_top` to 0).
 */
void term_scroll_abs(int win_top);

/**
 * Scroll the window downwards by the given number of lines.
 * If `lines` is negative, then scroll upwards. Alternative
 * interface to `term_scroll_abs()`.
 *
 * See `term_scroll_abs()` for a more thorough description
 * of the scrolling behavior.
 */
void term_scroll(int lines);

/**
 * Advance the cursor position. This will scroll the window
 * if the cursor was at the end of the window.
 */
void term_advance(void);

/**
 * Scroll the window to include the cursor.
 *
 * The default scroll behavior
 *
 * TODO(jlam55555): Define options to customize this
 *     behavior.
 */
void term_scroll_to_cursor(void);

#endif // TERMINAL_H
