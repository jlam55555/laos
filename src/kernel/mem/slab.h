/**
 * Slab allocators for small physical memory allocations.
 *
 * Use a LIFO freelist for O(1) allocations/deallocations as described in
 * https://www.kernel.org/doc/gorman/html/understand/understand011.html. Another
 * possible implementation is to use a simple bitmap/buddy system allocator,
 * which uses less space but doesn't guarantee O(1) allocations.
 *
 * A slab allocator allows for allocations of a power-of-2 order, from 2^5 to
 * 2^17. A slab allocator (`struct slab_cache`) contains three linked-lists of
 * slab descriptors (`struct slab`): one for full slabs, one for partially-full
 * slabs, and one for empty slabs (candidates for freeing). Each slab comprises
 * three parts:
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
 * The layout in memory is somewhat different for orders 5-9 and orders 10-17.
 * Orders 5-9 store their slab descriptor and LIFO freelist on the physical
 * backing page for the slab elements. Orders 10-17 store their slab descriptor
 * and LIFO freelist in memory chunks allocated using the lower-order slab
 * allocators. The latter works well because it causes less wasted space on the
 * physical backing pages, and the freelists are small because there are few
 * elements per slab.
 *
 * The slab allocators underlie the familiar `kmalloc()` interface, which
 * delegates the work to the appropriate slab allocator.
 *
 * When freeing an object, the slab that the object belongs to is noted by the
 * `struct page` for the physical page of the object memory.
 *
 * TODO(jlam55555): This needs extensive unit testing. As well as everything
 * else in this kernel.
 */
#ifndef MEM_SLAB_H
#define MEM_SLAB_H

#include <stddef.h>

#define SLAB_MIN_ORDER 5
#define SLAB_MAX_ORDER 17
#define SLAB_SMALL_MAX_ORDER 9
#define SLAB_LARGE_MIN_ORDER (SLAB_SMALL_MAX_ORDER + 1)

/**
 * TODO(jlam55555): Working here.
 */
struct slab {
  // TODO(jlam55555): Implement `struct list_head`.
  struct slab *next;
};
struct slab_cache {
  unsigned order;

  // These have length 1 to act as sentinel nodes.
  struct slab empty_slabs[1];
  struct slab partial_slabs[1];
  struct slab full_slabs[1];
};

/**
 * Create and initialize all slab allocators. This must be called before
 * performing allocations with `kmalloc()`.
 */
void slab_allocators_init(void);

/**
 * Allocate memory up to 128KB.
 *
 * Returns NULL if the memory size is too large or no memory can be allocated.
 */
void *kmalloc(size_t sz);

/**
 * Free a memory region allocated with `kmalloc()` or directly from a slab
 * allocator.
 */
void kfree(void *obj);

#endif // MEM_SLAB_H
