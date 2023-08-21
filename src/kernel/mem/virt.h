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
 * virt_mem_init() sets up the virtual memory manager, creates a new (4KiB)
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

/*
 * Similar to `struct vm_area_struct` in Linux. Represents a contiguous VM
 * region allocated (mapped) by a process via `mmap()`, but not necessarily all
 * mapped in the page table. The page fault handler will use this to determine
 * whether a nonpresent virtual address is valid and should be faulted in from
 * memory.
 *
 * These are maintained as a sorted linked-list of non-overlapping regions on a
 * `struct process`.
 */
struct vm_area {
  uint64_t base;
  uint64_t len;
  struct vm_area *next;
};

/**
 * Initialize the virtual memory manager. This performs the following steps:
 * 1. Initializes the physical memory manager with the initial mmap entries.
 * 2. Sets up a new page table with the memory map detailed above. Also keep the
 * bootloader-reclaimable entries for now. (It has some useful information, such
 * as the kernel stack.)
 * 3. Swaps to the new page table.
 * 4. Builds a new stack and jumps into it.
 * 5. Instructs the physical memory manager to reclaim the reclaimable memory
 * sections from the initial mmap entries.
 *
 * [[noreturn]] because we're at the top a new stack, anything on the stack
 * before will be obliterated.
 */
void virt_mem_init(struct limine_memmap_entry *init_mmap, size_t entry_count);

#endif // MEM_VIRT_H
