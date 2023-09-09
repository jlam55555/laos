/**
 * Physical memory manager (PMM). Currently, this PMM keeps a struct page array
 * which keeps track of some metadata for all the physical pages of RAM.
 * Allocation simply involves a sequential scan of this array until a free page
 * is found.
 *
 * This is clearly not fast. Augmenting this with a bitmap or buddy allocator
 * will allow for faster allocations. A LIFO allocator similar to the slab
 * allocator may also work, but this requires much more storage (O(log2(N))) per
 * physical page.
 *
 * Currently, there is not much metadata associated with each physical page, but
 * this may change in the future. E.g., refcounts/the number of free entries in
 * a PML* table may be helpful for freeing physical pages.
 *
 * External APIs take as argument and return HHDM virtual addresses. Internal
 * APIs take as argument and return physical (direct-mapped) addresses.
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

// Forward declarations. Mostly for extra information needed for different
// context bits.
struct slab;

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

  // Set if hole memory, or for bootloader-reclaimable memory before it has been
  // reclaimed.
  bool unusable : 1;

  // For future use.
  uint64_t : 62; // 8

  // Used to store metadata about the page. Depends on the type of page this is.
  // More entries may be added as more page types appear
  union {
    // The slab object, if this is a backing page for a slab.
    struct slab *slab; // 8
  } context;

  // For future use. Pad this to 64 bytes as is done in Linux.
  /* uint64_t test[6]; // 48 */
} __attribute__((packed));

/**
 * Initialize physical memory map using Limine memmap feature (which is itself
 * based on the BIOS E820 function).
 */
void phys_mem_init(struct limine_memmap_entry *init_mmap, size_t entry_count);

/**
 * Free bootloader-reclaimable (Limine) memory. This should only be done once
 * all dependencies on resources stored in bootloader-reclaimable memory are
 * replaced/unneeded (e.g., kernel stack, page table).
 */
void phys_reclaim_bootloader_mem(struct limine_memmap_entry *init_mmap,
                                 size_t entry_count);

/**
 * Allocate a single physical page using a round-robin allocator.
 *
 * Returns the address of the new page, or NULL if physical pages are exhausted.
 */
void *phys_page_alloc(void);

/**
 * Free a single physical page.
 */
void phys_page_free(void *pg);

/**
 * Allocate multiple contiguous pages. Simple but inefficient for order > 0.
 */
void *phys_page_alloc_order(unsigned order);

/**
 * Free multiple contiguous pages.
 */
void phys_page_free_order(void *pg, unsigned order);

/**
 * Print statistics about physical memory (e.g., available, reserved, usable,
 * etc.)
 */
void phys_mem_print_stats(void);

/**
 * Get `struct page` for a physical address. This currently doesn't do any
 * checks on the input address, but the return value should be checked in case
 * checks are added in the future.
 */
struct page *phys_get_page(void *pg);

#endif // MEM_PHYS_H
