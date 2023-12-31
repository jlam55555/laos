/**
 * x86_64-specific initialization.
 */
#ifndef ARCH_X86_64_INIT_H
#define ARCH_X86_64_INIT_H

/**
 * Called by kernel before setting up memory allocators and scheduler.
 *
 * For x86_64, we want to initialize the segment tables (GDT, IDT) and enable
 * desired features in MSRs.
 */
void arch_init(void);

#endif // ARCH_X86_64_INIT_H
