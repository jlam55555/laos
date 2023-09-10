/**
 * Slab allocators for small physical memory allocations.
 *
 * Use a LIFO freelist for O(1) allocations/deallocations as described in
 * https://www.kernel.org/doc/gorman/html/understand/understand011.html. This
 * requires an overhead of 2 bytes per object, plus the slab descriptor.
 * Compared to the SLUB allocator (described below), this has a relatively high
 * memory overhead; the benefit is that it does allow for caching initialized
 * objects.
 *
 * An alternate (and possibly better overall) design is that of the Linux SLUB
 * allocator, which instead stores slab metadata in the `struct page`, requires
 * no overhead per element, and maintains O(1) allocations/frees. Free elements
 * form a linked list (the freelist). However, this requires overwriting the
 * objects themselves, which makes this method unsuitable for caching
 * initialized objects. If implemented, much of the logic for the SLUB allocator
 * can be shared with this slab allocator.
 *
 * A slab allocator allows for allocations of a power-of-2 order, from
 * 2^SLAB_MIN_ORDER to 2^SLAB_MAX_ORDER. A slab allocator (`struct slab_cache`)
 * contains three linked-lists of slab descriptors (`struct slab`): one for full
 * slabs, one for partially-full slabs, and one for empty slabs (candidates for
 * freeing). Each slab comprises three parts:
 * - The slab descriptor (metadata about the slab).
 * - The LIFO freelist.
 * - The physical backing page(s) for the slab elements.
 *
 * The freelist is an array of 8-byte indices. If a slab of N total objects has
 * A allocated objects, then the first A elements of the freelist contain the
 * indices of the A allocated objects, and the remaining N-A elements of the
 * freelist contain the indices of the remaining N-A free elements. Allocations
 * within a slab simply mean allocating the next free element within a slab and
 * incrementing A (if there are none, then allocate another slab); deallocations
 * mean swapping the freed element with the last-allocated element in the
 * freelist and decrementing A.
 *
 * The layout in memory is somewhat different for "small" orders (SLAB_MIN_ORDER
 * <= order <= SLAB_SMALL_MAX_ORDER) and "large" orders (SLAB_LARGE_MIN_ORDER <=
 * order <= SLAB_MAX_ORDER). Small-order slabs store their slab descriptor and
 * LIFO freelist on the physical backing page for the slab elements. Large order
 * slabs store their slab descriptor and LIFO freelist in memory chunks
 * allocated using the lower-order slab allocators. The latter works well
 * because it causes less wasted space on the physical backing pages, and the
 * freelists are small because there are few elements per slab.
 *
 * The slab allocators underlie the familiar `kmalloc()` interface, which
 * delegates the work to the appropriate slab allocator.
 *
 * When freeing an object, the slab that the object belongs to is noted by the
 * `struct page` for the physical page of the object memory.
 *
 * Design-wise, there are three levels of abstraction (most to least abstract):
 *
 * 1. `kmalloc()`/`kfree()`
 * 2. `slab_cache_alloc()`/`slab_cache_free()`
 * 3. `_slab_alloc()`/`_slab_free()`
 *
 * The innermost layer is considered an implementation detail. Unit testing is
 * only done on the first two levels, which should fully exercise the
 * `_slab_*()` methods. `kmalloc()`/`kfree()` should be sufficient for most use
 * cases unless a custom slab allocator is needed.
 *
 * TODO(jlam55555): Introduce a `slab_cache_purge()` method that purges unused
 * slabs in the slablists. This will only be needed for low-memory situations so
 * it's not something we need to worry about in usual operation.
 */
#ifndef MEM_SLAB_H
#define MEM_SLAB_H

#include <stddef.h>
#include <stdint.h>

#include "common/list.h" // struct list_head
#include "mem/phys.h"    // struct phys_rra

#define SLAB_MIN_ORDER 4
#define SLAB_MAX_ORDER 16
#define SLAB_SMALL_MAX_ORDER 7
#define SLAB_LARGE_MIN_ORDER (SLAB_SMALL_MAX_ORDER + 1)

/**
 * See comment in slab.c.
 */
struct slab;

/**
 * Slab allocator for a particular order. Maintains a cache of `struct slab`s
 * for this order.
 */
struct slab_cache {
  unsigned order; // 4

  uint8_t pages;    // 1
  uint8_t elements; // 1

  // Precomputed offset from page start. Only useful for data in small-order
  // slabs. (Always 0 for large-order slabs.)
  uint16_t offset; // 2

  // Physical page allocator.
  struct phys_rra *allocator; // 8

  // Sentinel nodes for slab freelists. (Note that these are packed structs, so
  // the alignment may be a bit strange).
  struct list_head empty_slabs;
  struct list_head partial_slabs;
  struct list_head full_slabs;
};

/**
 * Create and initialize all slab allocators. This must be called before
 * performing allocations with `kmalloc()`.
 */
void slab_allocators_init(void);

/**
 * Allocate memory region up to 128KiB.
 *
 * Note that allocations >4KiB (PG_SZ) are not guaranteed to be fast or even to
 * succeed. This uses a very naive implementation for contiguous multi-physical
 * page allocations.
 *
 * Returns NULL if the memory size is too large or no memory can be allocated.
 */
void *kmalloc(size_t sz);

/**
 * Free a memory region allocated with `kmalloc()` or directly from a slab
 * allocator.
 */
void kfree(const void *obj);

/**
 * Initialize a `struct slab_cache`, including dynamically determining how many
 * elements should be in this slab_cache.
 *
 * Requirements for number of elements per slab of size S (usually S == PG_SZ,
 * but S must be larger if 2^M > PG_SZ):
 * - For small object slabs of order M:
 *   - sizeof(struct slab) + N + N*2^M <= S.
 *   - Note that the objects need to be 2^M aligned as well.
 * - For large object slabs of order M:
 *   - N*2^M <= S
 *   - sizeof(struct slab) + N <= 2^(M-1). (This means that the descriptor size
 *     can use a lower-order slab allocator.)
 *
 * In general, the goal is to minimize wasted space overhead. The number of
 * wasted bytes is computed by _slab_allocator_init. (Perhaps a better measure
 * of "wasted bytes" might be the ratio of allocatable bytes to the total
 * backing store size for one slab.)
 */
void slab_cache_init(struct slab_cache *slab_cache, struct phys_rra *rra,
                     unsigned order);

/**
 * Clean up a slab cache. This means cleaning up all slabs associated with this
 * slab cache.
 */
void slab_cache_destroy(struct slab_cache *slab_cache);

/**
 * Allocate a new slab for the provided slab cache, and add the new slab to the
 * slab cache's empty list.
 */
void slab_cache_alloc_slab(struct slab_cache *slab_cache);

/**
 * Allocate an object within a slab cache. O(1) runtime.
 */
void *slab_cache_alloc(struct slab_cache *slab_cache);

/**
 * Frees the object from the parent slab cache.
 *
 * The signature may look a little strange. The reason for supplying both
 * `slab_cache` and `slab` is that we may know the slab in the process of
 * finding the slab_cache when freeing an object. If the slab is provided (not
 * NULL), it'll be used; otherwise, it'll be looked up from the `struct page`.
 */
void slab_cache_free(struct slab_cache *slab_cache, struct slab *slab,
                     const void *obj);

#endif // MEM_SLAB_H
