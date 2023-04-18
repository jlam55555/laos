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

static void default_driver_init(struct term_driver *driver) {
  term_ringbuf_init(&driver->mts_buf);
  term_ringbuf_init(&driver->stm_buf);
}
static void default_handle_master_write(struct term_driver *driver, char *buf,
                                        size_t sz) {
  if (driver->echo) {
    driver->handle_slave_write(driver, buf, sz);
  }

  term_ringbuf_write(&driver->mts_buf, buf, sz);
}
static int
default_handle_master_read(__attribute__((unused)) struct term_driver *driver,
                           __attribute__((unused)) char *buf,
                           __attribute__((unused)) size_t sz) {
  /* Noop; default_handle_slave_write() will call the console driver */
  return 0;
}
static void
default_handle_slave_write(__attribute__((unused)) struct term_driver *driver,
                           char *buf, size_t sz) {
  /* Forwards the request to the console driver. */
  struct console_driver *console_driver = get_default_console_driver();
  console_driver->handle_write(console_driver, buf, sz);
}
static int default_handle_slave_read(struct term_driver *driver, char *buf,
                                     size_t sz) {
  return term_ringbuf_read(&driver->mts_buf, buf, sz);
}
static struct term_driver default_term_driver = {
    .echo = true,
    .driver_init = default_driver_init,
    .handle_master_write = default_handle_master_write,
    .handle_master_read = default_handle_master_read,
    .handle_slave_write = default_handle_slave_write,
    .handle_slave_read = default_handle_slave_read,
};

struct term_driver *get_default_term_driver(void) {
  static bool is_init = false;
  if (!is_init) {
    default_term_driver.driver_init(&default_term_driver);
    is_init = true;
  }
  return &default_term_driver;
}
