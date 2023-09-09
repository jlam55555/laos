#include "mem/slab.h"

#include "arch/x86_64/pt.h"
#include "common/libc.h"
#include "mem/phys.h"

#include <assert.h>

/**
 * Main slab caches.
 */
struct slab_cache _slab_caches[SLAB_MAX_ORDER - SLAB_MIN_ORDER + 1];

static bool _slab_cache_is_small(unsigned order) {
  return order <= SLAB_SMALL_MAX_ORDER;
}

/**
 * Helper function to get the main slab cache of the given order.
 */
static struct slab_cache *_slab_allocator_get_cache(unsigned order) {
  if (order < SLAB_MIN_ORDER || order > SLAB_MAX_ORDER) {
    return NULL;
  }
  return &_slab_caches[order - SLAB_MIN_ORDER];
}

/**
 * This function has a bit of a funny signature (which indicates that I should
 * really implement standardized LL macros.
 *
 * Usage: if you want to move slab from (its existing linked list) to
 * slab_cache->xyz_slabs, then call this like so:
 *
 *    slab_cache->xyz_slabs = _slab_move_to_ll(slab, slab_cache->xyz_slabs);
 *
 * The return value is for convenience. Alternatively, you can do:
 *
 *    _slab_move_to_ll(slab, slab_cache->xyz_slabs);
 *    slab_cache->xyz_slabs = slab;
 *
 * Maybe this should be a macro, and maybe I should add a sentinel node to the
 * slab LLs. It's not too big of a deal now but it might be when more LLs get
 * introduced.
 */
static struct slab *_slab_move_to_ll(struct slab *slab, struct slab *ll) {
  // Remove from current linked list.
  if (slab->prev) {
    slab->prev->next = slab->next;
  }
  if (slab->next) {
    slab->next->prev = slab->prev;
  }

  // Prepend to the new linked list.
  slab->prev = NULL;
  slab->next = ll;

  // For convenience; see comment above function.
  return slab;
}

/**
 * Courtesy of bit-twiddling hacks:
 * https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
 */
static unsigned _round_up_pow2(unsigned v) {
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  return v;
}

void slab_cache_init(struct slab_cache *slab_cache, struct phys_rra *rra,
                     unsigned order) {
  slab_cache->order = order;
  slab_cache->empty_slabs = NULL;
  slab_cache->partial_slabs = NULL;
  slab_cache->full_slabs = NULL;
  slab_cache->allocator = rra;

  const size_t element_size = 1u << order;
  size_t desc_size;
  __attribute__((unused)) size_t wasted;
  if (_slab_cache_is_small(order)) {
    slab_cache->pages = 1;
    slab_cache->elements = (PG_SZ - sizeof(struct slab)) / (element_size + 1);

    desc_size = sizeof(struct slab) + slab_cache->elements;
    wasted = PG_SZ - (desc_size + slab_cache->elements * element_size);
  } else {
    slab_cache->pages = order >= PG_SZ_BITS ? PG_COUNT(1u << order) : 1;
    slab_cache->elements = (slab_cache->pages << PG_SZ_BITS) >> order;

    // Descriptor + freelist must fit in the descriptor of a lower-level
    // freelist.
    desc_size = sizeof(struct slab) + slab_cache->elements;
    assert(desc_size <= (1u << (order - 1)));

    wasted = _round_up_pow2(desc_size) - desc_size;
  }

#ifdef DEBUG
  printf("slaballoc: order=%u pages=%u elements=%u small=%u desc_size=%lu "
         "wasted=%lu\r\n",
         slab_cache->order, slab_cache->pages, slab_cache->elements,
         _slab_cache_is_small(order), desc_size, wasted);
#endif // DEBUG
}

void slab_allocators_init(void) {
  for (unsigned order = SLAB_MIN_ORDER; order <= SLAB_MAX_ORDER; ++order) {
    slab_cache_init(_slab_allocator_get_cache(order), phys_mem_get_rra(),
                    order);
  }
}

void slab_cache_alloc_slab(struct slab_cache *slab_cache) {
  void *const page =
      phys_rra_alloc_order(slab_cache->allocator, slab_cache->order);
  if (!page) {
    return;
  }
  void *const page_hm = VM_TO_HHDM(page);

  const size_t desc_sz = sizeof(struct slab) + slab_cache->elements;
  const size_t element_sz = 1u << slab_cache->order;
  struct slab *slab;
  void *objects_start;
  if (_slab_cache_is_small(slab_cache->order)) {
    // Initialize desc in backing page.
    slab = (struct slab *)page_hm;
    objects_start =
        page_hm + (desc_sz + element_sz - 1) / element_sz * element_sz;
  } else {
    // Allocate descriptor.
    slab = (struct slab *)kmalloc(desc_sz);
    objects_start = page_hm;
  }

  if (!slab) {
    phys_rra_free_order(slab_cache->allocator, page, slab_cache->order);
    return;
  }

  // Initialize slab ll.
  slab->prev = NULL;
  slab->next = NULL;
  slab->data = objects_start;

  slab->parent = slab_cache;
  slab->allocated = 0;

  // Initialize freelist.
  for (uint8_t i = 0; i < slab_cache->elements; ++i) {
    slab->freelist[i] = i;
  }

  // Set reference to slab_cache in struct page.
  struct page *const pg_desc = phys_rra_get_page(slab_cache->allocator, page);
  assert(pg_desc);
  pg_desc->context.slab = slab;

  slab_cache->empty_slabs = slab;
}

struct slab *slab_cache_find_nonfull_slab(struct slab_cache *slab_cache) {
  // Look for partially-full slabs first.
  if (slab_cache->partial_slabs) {
    return slab_cache->partial_slabs;
  }

  // Look for empty slabs next.
  if (slab_cache->empty_slabs) {
    return slab_cache->empty_slabs;
  }

  // Need to allocate a new slab.
  slab_cache_alloc_slab(slab_cache);
  return slab_cache->empty_slabs;
}

void *slab_alloc(struct slab *slab) {
  assert(slab);
  assert(slab->allocated < slab->parent->elements);

  const size_t order_sz = 1u << slab->parent->order;
  void *const obj = slab->data + slab->freelist[slab->allocated] * order_sz;
  ++slab->allocated;
  return obj;
}

void *slab_cache_alloc(struct slab_cache *slab_cache) {
  struct slab *const slab = slab_cache_find_nonfull_slab(slab_cache);
  if (!slab) {
    return NULL;
  }

  bool is_slab_orig_empty = !!slab->allocated;

  // Perform allocation.
  void *obj = slab_alloc(slab);

  if (slab->allocated == slab->parent->elements) {
    // Move to full list.
    slab_cache->full_slabs = _slab_move_to_ll(slab, slab_cache->full_slabs);
  } else if (is_slab_orig_empty) {
    // Move to partially-full list.
    slab_cache->partial_slabs =
        _slab_move_to_ll(slab, slab_cache->partial_slabs);
  }

  return obj;
}

void *kmalloc(size_t sz) {
  // TODO(jlam55555): Write a function for ilog2 using bsr.
  int order = -1;
  while (sz) {
    sz >>= 1;
    ++order;
  }

  if (order < SLAB_MIN_ORDER) {
    order = SLAB_MIN_ORDER;
  }
  struct slab_cache *const slab_cache = _slab_allocator_get_cache(order);
  if (!slab_cache) {
    return NULL;
  }
  return slab_cache_alloc(slab_cache);
}

void slab_free(struct slab *slab, const void *obj) {
  // Find the position of the object in the freelist.
  const size_t off = obj - slab->data;
  const unsigned order = slab->parent->order;

  // Assert that the offset is aligned to the size of the object.
  assert(!(off & ((1u << order) - 1)));

  // Find index of the allocated element in the freelist.
  const uint8_t index = off >> order;
  uint8_t i;
  for (i = 0; i < slab->parent->elements && slab->freelist[i] != index; ++i) {
  }
  assert(i < slab->parent->elements);

  // Free the element 1. Beforehand:
  //
  //          used   |  free
  //     | 1 | 3 | 4 | 5 | 2 |
  //
  //       used  |   free
  //     | 4 | 3 | 1 | 5 | 2 |
  //
  // Afterwards. Note that 1 (the element to be freed) and 4 (the last used
  // element) swap places.
  --slab->allocated;
  slab->freelist[i] = slab->freelist[slab->allocated];
  slab->freelist[slab->allocated] = index;
}

void slab_cache_free(struct slab *slab, const void *obj) {
  struct slab_cache *const slab_cache = slab->parent;

  bool is_slab_orig_full = slab->allocated == slab->parent->elements;

  // Perform freeing.
  slab_free(slab, obj);

  if (!slab->allocated) {
    // Move to empty list.
    slab_cache->empty_slabs = _slab_move_to_ll(slab, slab_cache->empty_slabs);
  } else if (is_slab_orig_full) {
    // Move to partially-full list.
    slab_cache->partial_slabs =
        _slab_move_to_ll(slab, slab_cache->partial_slabs);
  }
}

void kfree(const void *obj) {
  struct page *const pg = phys_rra_get_page(phys_mem_get_rra(), VM_TO_IDM(obj));
  assert(pg);

  struct slab *const slab = pg->context.slab;
  assert(slab);

  slab_cache_free(slab, obj);
}
