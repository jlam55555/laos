/**
 * Utilities for traveling between kernel-space and user-space.
 *
 * I want to call this `portal.h`, but I'm sticking to tradition and calling
 * this `entry.h` instead.
 */
#ifndef ARCH_X86_64_ENTRY_H
#define ARCH_X86_64_ENTRY_H

/**
 * Jump into userspace. This performs an `iret` as described in
 * https://wiki.osdev.org/Getting_to_Ring_3#Entering_Ring_3.
 */
void arch_jump_userspace(void (*cb)(void));

#endif // ARCH_x86_64_ENTRY_H
