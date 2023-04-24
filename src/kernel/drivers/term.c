#include "drivers/term.h"
#include "common/libc.h"
#include "drivers/console.h"

void term_ringbuf_init(struct term_ringbuf *rb) {
  rb->size = 0;
  rb->head = 0;
}
size_t term_ringbuf_write(struct term_ringbuf *rb, char *buf, size_t sz) {
  size_t i;
  for (i = 0; i < sz && rb->size < TERM_BUF_SIZE;
       ++i, ++rb->size, rb->head %= TERM_BUF_SIZE) {
    rb->buf[rb->head++] = *buf++;
  }
  return i;
}
size_t term_ringbuf_read(struct term_ringbuf *rb, char *buf, size_t sz) {
  size_t i;
  for (i = 0; i < sz && rb->size; ++i, --rb->size) {
    *buf++ = rb->buf[(rb->head - rb->size + TERM_BUF_SIZE) % TERM_BUF_SIZE];
  }
  return i;
}

static char *_ctrl_chars[31] = {
    "^@", "^A", "^B", "^C", "^D", "^E", "^F", "^G", "^H", "^I", "^J",
    "^K", "^L", "^M", "^N", "^O", "^P", "^Q", "^R", "^S", "^T", "^U",
    "^V", "^W", "^X", "^Y", "^Z", "^[", "^]", "^^", "^_",
};

// Currently implementing a very simple raw-mode (i.e., no ldisc/cooked mode)
// terminal.
static void _master_write(struct term *term, char *buf, size_t sz) {
  if (term->echo) {
    // Convert control sequences to the carat (^X) form.
    // We don't expect ASCII values less than zero, but we print
    // them out if we do. Otherwise we would be accessing negative
    // indices in the _ctrl_chars array.
    for (size_t i = 0; i < sz; ++i) {
      if (isprint(buf[i]) || buf[i] < 0) {
        term->driver->slave_write(term, buf + i, 1);
      } else {
        term->driver->slave_write(term, _ctrl_chars[buf[i]], 2);
      }
    }
  }

  term_ringbuf_write(&term->mts_buf, buf, sz);
}
static int _master_read(__attribute__((unused)) struct term *term,
                        __attribute__((unused)) char *buf,
                        __attribute__((unused)) size_t sz) {
  /* Noop; _slave_write() will call the console driver */
  return 0;
}
static void _slave_write(__attribute__((unused)) struct term *term, char *buf,
                         size_t sz) {
  /* Forwards the request to the console driver. */
  struct console_driver *console_driver = get_default_console_driver();
  console_driver->write(console_driver->dev, buf, sz);
}
static int _slave_read(struct term *term, char *buf, size_t sz) {
  return term_ringbuf_read(&term->mts_buf, buf, sz);
}
static struct term _term_device = {
    .echo = true,
};
static void _driver_init(struct term_driver *driver) {
  driver->dev = &_term_device;
  driver->dev->driver = driver;
  term_ringbuf_init(&driver->dev->mts_buf);
  term_ringbuf_init(&driver->dev->stm_buf);
}
static struct term_driver default_term_driver = {
    .driver_init = _driver_init,
    .master_write = _master_write,
    .master_read = _master_read,
    .slave_write = _slave_write,
    .slave_read = _slave_read,
};

struct term_driver *get_default_term_driver(void) {
  static bool is_init = false;
  if (!is_init) {
    default_term_driver.driver_init(&default_term_driver);
    is_init = true;
  }
  return &default_term_driver;
}
