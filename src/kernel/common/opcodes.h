/**
 * A C API to common opcodes. The op_* interfaces are meant for use outside of
 * arch/, and the arch_* interfaces are meant for use within arch/. Since
 * they're aliases it doesn't really hurt to violate this convention, but it is
 * best if we have as few direct references to architecture-specific interfaces
 * from architecture-agnostic code as possible.
 *
 * Since these are just aliases, there is no "real" type enforcement, at least
 * not across different architectures. We only have x86_64 right now, so it's
 * not really a problem, but it may become a problem if there are discrepancies
 * if/once we support multiple architectures.
 *
 * These interfaces are named after the x86_64 opcodes.
 */
#ifndef COMMON_OPCODES_H
#define COMMON_OPCODES_H

#include "arch/x86_64/opcodes.h" // for arch_*

#define op_hlt arch_hlt // Wait for interrupt.
#define op_sti arch_sti // Enable IRQs.
#define op_cli arch_cli // Disable IRQs.

#define op_outb arch_outb // Write one byte to a port.
#define op_outw arch_outw // Write one word to a port.
#define op_inb arch_inb   // Read one byte from a port.

#define op_rdtsc arch_rdtsc // Read HW timestamp counter.

#define op_bsr arch_bsr // Bit-Scan Reverse.

#endif // COMMON_OPCODES_H
