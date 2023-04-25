#include "diag/shell.h"

#include "common/libc.h"
#include "drivers/console.h"
#include "drivers/term.h"

static struct term_driver *_term_driver;
static char _input_buf[4097];
size_t _input_size;

/**
 * Display shell prompt.
 */
void _prompt(void) { printf("$ "); }

/**
 * Enqueue a byte onto the prompt. Do a very rough treatment of ^M and ^H.
 */
void _enqueue_byte(char c) {
  if (c == '\b') {
    if (_input_size) {
      --_input_size;
    }
  } else if (c == '\r') {
    _input_size = 0;
  } else if (_input_size < sizeof _input_buf) {
    _input_buf[_input_size++] = c;
  }
}

/**
 * Respond to the input.
 *
 * TODO(jlam55555): Note that the input string cannot have ^J, ^M, nor ^H
 *    characters due to their special treatment at the moment. Also, it may
 *    have null characters within, which may force us not to use printf.
 */
void _handle_input() {
  _input_buf[_input_size] = 0;
  printf("\rGot some input of length %d: \"%s\"\r\n", _input_size, _input_buf);
  _input_size = 0;
}

void shell_init() {
  _term_driver = get_default_term_driver();
  _input_size = 0;
  _prompt();
}

void shell_on_interrupt() {
  static char buf[TERM_BUF_SIZE];
  size_t bytes;
  if (!(bytes =
            _term_driver->slave_read(_term_driver->dev, buf, sizeof(buf)))) {
    return;
  }

  // Handle input.
  for (size_t i = 0; i < bytes; ++i) {
    if (buf[i] == '\n') {
      _handle_input();
      _prompt();
    } else {
      _enqueue_byte(buf[i]);
    }
  }
}
