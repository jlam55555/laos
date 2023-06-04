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

#define PG_SZ 4096lu
#define PG_SZ_BITS 12u

// Round up sz to the nearest page value.
#define PG_CEIL(sz) (void *)(((size_t)(sz) + PG_SZ - 1) & ~(PG_SZ - 1))
#define PG_FLOOR(sz) (void *)((size_t)(sz) & ~(PG_SZ - 1))

// Number of pages needed to contain sz.
// (Equivalent to (PG_CEIL(sz) >> PG_SZ_BITS)).
#define PG_COUNT(sz) (((size_t)(sz) + PG_SZ - 1) >> PG_SZ_BITS)

// Check if sz is page-aligned.
#define PG_ALIGNED(sz) (!((size_t)(sz) & (PG_SZ - 1)))

// Bitmap functions.
#define _BM_BYTE(bm, bit) ((bm)[(bit) >> 3])
#define _BM_BIT(bit) (1u << ((bit)&0x7))
#define BM_TEST(bm, bit) (_BM_BYTE(bm, bit) & _BM_BIT(bit))
#define BM_SET(bm, bit) (_BM_BYTE(bm, bit) |= _BM_BIT(bit))
#define BM_CLEAR(bm, bit) (_BM_BYTE(bm, bit) &= ~_BM_BIT(bit))

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
