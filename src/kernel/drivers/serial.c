#include "drivers/serial.h"

#include "common/util.h"

// Copied from https://wiki.osdev.org/Serial_Ports

#define SERIAL_PORT 0x3f8 // COM1

int serial_init(void) {
  outb(0x00, SERIAL_PORT + 1); // Disable all interrupts
  outb(0x80, SERIAL_PORT + 3); // Enable DLAB (set baud rate divisor)
  outb(0x03, SERIAL_PORT + 0); // Set divisor to 3 (lo byte) 38400 baud
  outb(0x00, SERIAL_PORT + 1); //                  (hi byte)
  outb(0x03, SERIAL_PORT + 3); // 8 bits, no parity, one stop bit
  outb(0xC7,
       SERIAL_PORT + 2); // Enable FIFO, clear them, with 14-byte threshold
  outb(0x0B, SERIAL_PORT + 4); // IRQs enabled, RTS/DSR set
  outb(0x1E, SERIAL_PORT + 4); // Set in loopback mode, test the serial chip
  outb(0xAE, SERIAL_PORT + 0); // Test serial chip (send byte 0xAE and check if
                               // serial returns same byte)

  // Check if serial is faulty (i.e: not same byte as sent)
  if (inb(SERIAL_PORT + 0) != 0xAE) {
    return 1;
  }

  // If serial is not faulty set it in normal operation mode
  // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
  outb(0x0F, SERIAL_PORT + 4);
  return 0;
}

static int _serial_is_transmit_empty(void) {
  return inb(SERIAL_PORT + 5) & 0x20;
}

void serial_putchar(char a) {
  while (_serial_is_transmit_empty() == 0) {
  }

  outb(a, SERIAL_PORT);
}
