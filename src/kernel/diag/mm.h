/**
 * Diagnostics related to memory mapping.
 *
 * Default page table as I understand it:
 *
 * Virtual address -> physical address
 * - 0xffffffff80000000 -> where the kernel is loaded, size
 *   is the size of the kernel in memory
 *   This gives us a constant offset for the kernel, and
 *   limits the kernel to 2GB. (This virtual address is 2GB
 *   less than the highest 64 bit address.) This is fixed size
 *   and should never expand (no memory allocation here).
 * - 0xffff800000000000 -> 0 (4GB)
 *   This maps the beginning of physical memory to the start
 *   of the kernel address space. (This virtual address is
 *   halfway through the possible valid (canonical) addresses
 *   in the 48-bit address space). This gives us plenty of room
 *   to allocate virtual memory as needed.
 * - 0x1000 -> 0x1000 (4GB)
 *   This provides a direct mapping for low memory, which is
 *   useful for things like the page table. The first (virtual) page
 *   is not mapped, presumably because of the null pointer (but there
 *   may also be additional significance to low virtual addresses that
 *   I'm not aware of). This direct memory is useful for data structures
 *   that need to be set up in physical memory, such as the page
 *   table itself.
 *
 * There is going to be some overlap as each of these
 * mappings will point to low-memory. So there may be as many as
 * three mappings per memory page.
 */
#ifndef DIAG_MM_H
#define DIAG_MM_H

void print_mm(void);

#endif // DIAG_MM_H
