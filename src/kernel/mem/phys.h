/**
 * Physical memory manager. Simple bitmap allocator, may
 * make this into a buddy allocator in the future.
 *
 * Most of these functions, except for phys_mem_print_stats(),
 * should only be called by the virtual memory manager.
 */
#ifndef MEM_PHYS_H
#define MEM_PHYS_H

#include <limine.h>
#include <stdbool.h>
#include <stddef.h>

#define PG_SZ 4096
#define PG_SZ_BITS 12

// Round up sz to the nearest page value.
#define PG_CEIL(sz) ((sz + PG_SZ - 1) & (PG_SZ - 1))

// Number of pages needed to contain sz.
#define PG_COUNT(sz) ((sz + PG_SZ - 1) >> PG_SZ_BITS)

// Check if sz is page-aligned.
#define PG_ALIGNED(sz) (!(sz & (PG_SZ - 1)))

/**
 * Initialize physical memory map using Limine memmap
 * feature (which is itself based on the BIOS E820 function).
 */
void phys_mem_init(struct limine_memmap_entry *init_mmap, size_t entry_count);

/**
 * Allocate a single physical page.
 *
 * Returns the physical address of the new page, or NULL
 * if physical pages are exhausted.
 */
void *phys_page_alloc(void);

/**
 * Free a single physical page.
 *
 * If pg is not valid (i.e., not allocated or beyond the
 * memory limit) then this will return false.
 */
bool phys_page_free(void *pg);

/**
 * Print statistics about physical memory (e.g., available,
 * reserved, usable, etc.)
 */
void phys_mem_print_stats(void);

#endif // MEM_PHYS_H
