#include "term.h"
#include "console.h"

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

static void _handle_master_write(struct term *term, char *buf, size_t sz) {
  if (term->echo) {
    term->driver->handle_slave_write(term, buf, sz);
  }

  term_ringbuf_write(&term->mts_buf, buf, sz);
}
static int _handle_master_read(__attribute__((unused)) struct term *term,
                               __attribute__((unused)) char *buf,
                               __attribute__((unused)) size_t sz) {
  /* Noop; default_handle_slave_write() will call the console driver */
  return 0;
}
static void _handle_slave_write(__attribute__((unused)) struct term *term,
                                char *buf, size_t sz) {
  /* Forwards the request to the console driver. */
  struct console_driver *console_driver = get_default_console_driver();
  console_driver->handle_write(console_driver->dev, buf, sz);
}
static int _handle_slave_read(struct term *term, char *buf, size_t sz) {
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
    .handle_master_write = _handle_master_write,
    .handle_master_read = _handle_master_read,
    .handle_slave_write = _handle_slave_write,
    .handle_slave_read = _handle_slave_read,
};

struct term_driver *get_default_term_driver(void) {
  static bool is_init = false;
  if (!is_init) {
    default_term_driver.driver_init(&default_term_driver);
    is_init = true;
  }
  return &default_term_driver;
}
