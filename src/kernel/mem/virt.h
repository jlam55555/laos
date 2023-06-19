/**
 * Virtual memory manager (VMM). Creates and maintains the page table data
 * structure, calling into the physical memory allocator to allocate/free
 * physical pages.
 *
 * This creates a page table with the following memory map. This assumes a
 * x86_64 system with a 48-bit VM addr space. The memory map is very similar to
 * Limine's pt, except that it does not include bootloader-only entries (and it
 * is not located in bootloader-reclaimable memory).
 *
 * - kernel map: virt 0xffffffff80000000 maps to phys <start of kernel> with
 * size <size of kernel>. This is the same as the Limine spec, and is used so
 * that any initialized global pointers to statically-allocated memory (e.g.,
 * for the console) remains valid. This virtual address space is limited to
 * 4GiB, but the kernel text+data+bss size is realistically much lower than
 * that.
 * - HHDM: virt 0xffff800000000000 maps to phys 0x0, with size <size of phys
 * mem>. This is a linear mapping of all physical memory to the beginning of
 * high memory (0xffff800000000000 is the first valid canonical address in high
 * memory.) A similar mapping is set in the Limine spec, although it exactly
 * 4GiB. High memory is limited to 2^47=128TiB, but realistically we're dealing
 * with much smaller RAM sizes.
 *
 * An identity map is not provided; use of the HHDM is preferred. The identity
 * map is only really useful before the initial page table is set up.
 *
 * virt_mem_init() is sets up the virtual memory manager, creates a new (4KiB)
 * kernel stack, and jump into that new stack. This is necessary because the old
 * kernel stack is located in bootloader-reclaimable memory (it is also large;
 * we follow the Linux convention that kernel stacks are 4KiB/1pg).
 *
 * N.B. The VMM will start reaching problems once we reach 128TiB (half of the
 * virtual address space) of RAM due to the HM restriction, but the PMM will
 * have problems sooner. See phys.h for a description of this problem. Suffice
 * it to say that we shouldn't worry about the VMM memory issues anytime soon.
 * Note that this is also far less than the size of the physical memory address
 * space (2^52=4PiB). This limit can be increased if we switch to 5-level paging
 * (which increases the virtual address space to 128PiB). Like Linux, we use a
 * HHDM and will suffer if the physical address size exceeds the virtual address
 * space.
 */
#ifndef MEM_VIRT_H
#define MEM_VIRT_H

#include <limine.h>
#include <stddef.h>

/**
 * Initialize the virtual memory manager. This performs the following steps:
 * 1. Initializes the physical memory manager with the initial mmap entries.
 * 2. Sets up a new page table with the memory map detailed above. Also keep the
 * bootloader-reclaimable entries for now. (It has some useful information, such
 * as the kernel stack.)
 * 3. Swaps to the new page table.
 * 4. Instructs the physical memory manager to reclaim the reclaimable memory
 * sections from the initial mmap entries.
 */
void virt_mem_init(struct limine_memmap_entry *init_mmap, size_t entry_count);

/**
 * Reclaim bootloader-reclaimable memory.
 *
 * This has the subtle (?) implication that the kernel stack will be discarded,
 * since it lies in bootloader-reclaimable memory. As a result, a new kernel
 * stack will be allocated and jumped into at the end of this function.
 *
 * This could be folded into virt_mem_init(), but is kept separate to be more
 * flexible.
 */
/* /\* __attribute__((noreturn)) *\/ void */
/* virt_mem_reclaim(struct limine_memmap_entry *init_mmap, size_t
   entry_count, */
/*                  void *(cb)(void)); */

void *virt_mem_map(void *phys_ptr, size_t pg);
void *virt_mem_map_noalloc(void *phys_ptr, size_t pg);

#endif // MEM_VIRT_H
