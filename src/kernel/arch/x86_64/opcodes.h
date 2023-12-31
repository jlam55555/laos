/**
 * Architecture-specific opcodes for common/opcodes.h.
 *
 * Architecture-agnostic code should include the common/opcodes.h header and use
 * the op_*() interfaces instead, in order to reduce outside dependency on
 * architecture-specific code.
 */
#ifndef ARCH_X86_64_OPCODES_H
#define ARCH_X86_64_OPCODES_H

#include "stdint.h"

// Nullary opcodes -- these can be written as the opcode themselves.
#define arch_hlt() __asm__("hlt")
#define arch_sti() __asm__("sti")
#define arch_cli() __asm__("cli")

void arch_outb(uint8_t value, uint16_t port);
void arch_outw(uint16_t value, uint16_t port);
uint8_t arch_inb(uint16_t port);

uint64_t arch_readtsc(void);

uint64_t arch_bsr(uint64_t n);

#endif // ARCH_X86_64_OPCODES_H
