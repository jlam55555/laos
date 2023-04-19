/**
 * The terminal driver provides a bidirectional I/O mechanism from
 * a hardware device (master side) to a process (slave side).
 *
 * The terminal uses two circular queues to buffer data (one
 * master-to-slave, and the other slave-to-master). Once a queue
 * is exhausted, further input is discarded upon write.
 *
 * This provides handlers to read from/write to either side of
 * the channel. Note that reads can be a noop if the read is never
 * manually called by the client, or is not called directly through
 * the terminal driver (but instead called at some higher layer
 * in the stack, such as a devfs filesystem abstaction layer).
 */
#ifndef TERM_H
#define TERM_H

#include <stdbool.h>
#include <stddef.h>

#define TERM_BUF_SIZE 4096

/**
 * Circular queue (ring buffer) data structure with a fixed size.
 *
 * These are represented using the head pointer and a size. Another
 * possible representation is a head and tail pointer, but using
 * head/size makes the size computation a little easier and removes
 * the ambiguity when head == tail (is the ringbuf empty or full?).
 */
struct term_ringbuf {
  size_t head, size;
  char buf[TERM_BUF_SIZE];
};
void term_ringbuf_init(struct term_ringbuf *);
size_t term_ringbuf_write(struct term_ringbuf *, char *, size_t);
size_t term_ringbuf_read(struct term_ringbuf *, char *, size_t);

struct term_driver;
struct term {
  // Echoing is on/off.
  bool echo;

  // Master-to-slave and slave-to-master queues.
  struct term_ringbuf mts_buf;
  struct term_ringbuf stm_buf;

  struct term_driver *driver;
};

struct term_driver {
  struct term *dev;
  void (*driver_init)(struct term_driver *);
  void (*handle_master_write)(struct term *, char *, size_t);
  int (*handle_master_read)(struct term *, char *, size_t);
  void (*handle_slave_write)(struct term *, char *, size_t);
  int (*handle_slave_read)(struct term *, char *, size_t);
};

// TODO: make it into a factory function that
// can perform init if necessary.
struct term_driver *get_default_term_driver(void);

#endif // TERM_H
