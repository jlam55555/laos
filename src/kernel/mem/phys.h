/**
 * Physical memory manager (PMM). Simple bitmap allocator, may make this into a
 * buddy allocator in the future.
 *
 * Most of these functions, except for phys_mem_print_stats(), should only be
 * called by the virtual memory manager so that they are properly mapped into
 * VM.
 *
 * N.B. Assuming `sizeof(struct page) == 64` like it is in Linux, we will start
 * running into problems with this allocator and the stock Linux
 * bootloader/protocol once the size of the `struct page` array exceeds the HHDM
 * size (4GiB). This will happen once we reach 256GiB of RAM, which is not that
 * unreasonable of a number today. The VMM will also have problems with large
 * memory due to the size of the VM space.
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
// TODO(jlam55555): Move these into some util library. These were used for the
// page bitmap when it was a bitmap, but now it is not a bitmap.
#define _BM_BYTE(bm, bit) ((bm)[(bit) >> 3])
#define _BM_BIT(bit) (1u << ((bit)&0x7))
#define BM_TEST(bm, bit) (_BM_BYTE(bm, bit) & _BM_BIT(bit))
#define BM_SET(bm, bit) (_BM_BYTE(bm, bit) |= _BM_BIT(bit))
#define BM_CLEAR(bm, bit) (_BM_BYTE(bm, bit) &= ~_BM_BIT(bit))

// Useful constants.
// TODO(jlam55555): Move these into some util library.
#define KiB 1024llu
#define MiB (KiB * KiB)
#define GiB (MiB * KiB)
#define TiB (GiB * KiB)
#define PiB (TiB * KiB)

/**
 * Used to track information about each physical memory page. Linux has a struct
 * of the same name with the same purpose.
 *
 * This struct may grow as amount of tracked data needed grows. In Linux,
 * `sizeof(struct page) == 64`, which means that 64/4096 ~= 1.5% of the total
 * physical memory goes towards the struct page array.
 *
 * `struct page`s will be zero-initialized by the physical memory allocator.
 */
struct page {
  // Set if allocated by the physical memory allocator.
  bool present : 1;

  // Set if hole memory.
  bool unusable : 1;

  // For future use. We may expand the size of the `struct page` if necessary.
  uint8_t : 6;
} __attribute__((packed));

/**
 * Initialize physical memory map using Limine memmap feature (which is itself
 * based on the BIOS E820 function).
 */
void phys_mem_init(struct limine_memmap_entry *init_mmap, size_t entry_count);

/**
 * Allocate a single physical page using a round-robin allocator.
 *
 * Returns the physical address of the new page, or NULL if physical pages are
 * exhausted.
 */
void *phys_page_alloc(void);

/**
 * Free a single physical page.
 */
void phys_page_free(void *pg);

/**
 * Print statistics about physical memory (e.g., available, reserved, usable,
 * etc.)
 */
void phys_mem_print_stats(void);

#endif // MEM_PHYS_H
