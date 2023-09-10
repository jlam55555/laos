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
 * `phys_rra_*()` methods are the lower-level interface for the round-robin page
 * allocator, and are mostly exposed for unit testing. The RRA interface expects
 * its arguments to be physical (identity-mapped) addresses, and will return
 * physical addresses.
 *
 * The non-RRA methods form a wrapper around the RRA methods. These interact
 * with the main memory allocator and expect/return HHDM addresses. These should
 * be used for most normal operation.
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
  // More entries may be added as more page types appear.
  union {
    // The slab object, if this is a backing page for a slab.
    struct slab *slab; // 8
  } context;

  // For future use. Pad this to 64 bytes as is done in Linux.
  /* uint64_t test[6]; // 48 */
} __attribute__((packed));

/**
 * Physical memory round-robin (page) allocator (RRA).
 */
struct phys_rra {
  /**
   * Store information about each `struct page`.
   *
   * Will be a HM address, since the new PT won't include a LM identity map.
   *
   * N.B. This was previously a bitmap that only stored whether each physical
   * page was allocated, but now we want to store more more info per page.
   */
  struct page *mem_bitmap;

  /**
   * Physical memory statistics. total_sz == total_pg * PG_SZ, included for
   * convenience. Total size is the end of the physical address space.
   *
   * Note that unusable_pg is counted in the total_pg but not the allocated_pg.
   * That is, total_pg = allocated_pg + unusable_pg + <free pg>. This definition
   * may change in the future if a more convenient definition arises.
   *
   * Note also that unusable_pg includes bootloader reclaimable memory before it
   * has been freed. This choice is somewhat arbitrary -- these pages could
   * alternatively have been marked as allocated to prevent their use.
   */
  size_t total_sz;
  size_t total_pg;
  size_t allocated_pg;
  size_t unusable_pg;

  /**
   * Round-robin (first-fit) allocation.
   */
  size_t needle;

  /**
   * Offset of the physical memory backing this allocator. This should be 0 in
   * the main allocator (since the backing buffer is the true physical memory),
   * and should be set to the offset of the backing buffer used in a testing
   * situation. This offset will be added/subtracted to the returned physical
   * address on allocation/frees.
   */
  void *phys_offset;
};

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
void *phys_alloc_page(void);

/**
 * Free a single physical page.
 */
void phys_free_page(const void *pg);

/**
 * Print statistics about physical memory (e.g., available, reserved, usable,
 * etc.)
 */
void phys_mem_print_stats(void);

/**
 * Gets a reference to the main RRA. For advanced cases (e.g., used in the slab
 * allocator) and unit testing.
 */
struct phys_rra *phys_mem_get_rra(void);

/**
 * Allocates a physical page. Errors if the page is unusable (e.g., hole
 * memory). Returns true iff the physical page is free.
 */
bool phys_rra_alloc(struct phys_rra *, const void *pg);

/**
 * Frees a physical page. Errors if the page is unusable (e.g., hole memory).
 * Returns true iff the physical page is allocated.
 */
bool phys_rra_free(struct phys_rra *, const void *pg);

/**
 * Initializes a RRA. Initializes the `struct page` array using the provided
 * `init_mmap`.
 *
 * Addresses in `init_mmap` are allowed to be in the HHDM, for convenience.
 */
void phys_rra_init(struct phys_rra *, void *addr, size_t mem_limit,
                   struct limine_memmap_entry *init_mmap, size_t entry_count,
                   void *phys_offset);

/**
 * Allocates/frees a continuous region of 2^order pages. Returns NULL if no such
 * region is found.
 *
 * Is not designed to be efficient for order > 0, although this can be improved
 * with a buddy allocator system.
 */
void *phys_rra_alloc_order(struct phys_rra *, unsigned order);
void phys_rra_free_order(struct phys_rra *, const void *pg, unsigned order);

/**
 * Returns the `struct page` associated with a page.
 */
struct page *phys_rra_get_page(struct phys_rra *rra, const void *pg);

#endif // MEM_PHYS_H
