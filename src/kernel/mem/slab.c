#include "mem/slab.h"

#include "common/libc.h"
#include "common/list.h"
#include "common/util.h" // for static_assert
#include "mem/phys.h"    // for phys_*
#include "mem/vm.h"      // for VM_TO_HHDM, VM_TO_IDM

#include <assert.h>

/**
 * Slab descriptor. This is stored one of the linked-lists in a struct
 * slab_cache allocator. The slab descriptor physically either resides at the
 * beginning of the physical backing page of the slab (for small-order slabs) or
 * was allocated using a lower-order slab allocator (for large-order slabs).
 *
 * This is exposed as an opaque type (hence why this isn't named `struct
 * _slab`). The slab allocator exposes `kmalloc()`/`kfree()` and operations on
 * `struct slab_cache`s, but not on `struct slab`s directly. This is considered
 * an implementation detail.
 */
struct slab {
  struct slab_cache *parent; // 8
  void *data;                // 8
  struct list_head ll;       // 16
  uint8_t allocated;         // 1
  uint64_t : 56;             // 7

  // This must come last.
  struct slab_freelist_item {
    // freelist[i].stack_item gets the i-th index on the stack. Used for
    // allocations.
    uint8_t stack_item;
    // freelist[i].pos_in_stk gets the index in the stack of the i-th object in
    // the slab. Used for deallocations.
    uint8_t pos_in_stk;
  } freelist[0];
};

/**
 * Ensure that slab_freelist_item is at the end of the `struct_slab`, without
 * requiring `__attribute__((packed))`. This is to prevent gcc from complaining
 * about bad alignment.
 */
static_assert(sizeof(struct slab) == 40);

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

void slab_cache_init(struct slab_cache *slab_cache, struct phys_rra *rra,
                     unsigned order) {
  slab_cache->order = order;

  list_init(&slab_cache->empty_slabs);
  list_init(&slab_cache->partial_slabs);
  list_init(&slab_cache->full_slabs);

  slab_cache->allocator = rra;

  const size_t element_size = 1u << order;
  size_t desc_size;
  __attribute__((unused)) size_t wasted;
  if (_slab_cache_is_small(order)) {
    slab_cache->pages = 1;
    slab_cache->elements = (PG_SZ - sizeof(struct slab)) /
                           (element_size + sizeof(struct slab_freelist_item));

    desc_size = sizeof(struct slab) +
                slab_cache->elements * sizeof(struct slab_freelist_item);
    wasted = PG_SZ - (desc_size + slab_cache->elements * element_size);
  } else {
    slab_cache->pages = order >= PG_SZ_BITS ? PG_COUNT(1u << order) : 1;
    slab_cache->elements = (slab_cache->pages << PG_SZ_BITS) >> order;

    // Descriptor + freelist must fit in the descriptor of a lower-level
    // freelist.
    desc_size = sizeof(struct slab) +
                slab_cache->elements * sizeof(struct slab_freelist_item);
    assert(desc_size <= (1u << (order - 1)));

    wasted = (1u << ilog2ceil(desc_size)) - desc_size;
  }

#ifdef DEBUG
  printf("slaballoc: order=%u pages=%u elements=%u small=%u desc_size=%lu "
         "wasted=%lu\r\n",
         slab_cache->order, slab_cache->pages, slab_cache->elements,
         _slab_cache_is_small(order), desc_size, wasted);
#endif // DEBUG
}

void _slab_destroy(struct slab *slab) {
  // Deallocate the backing pages. For small-order slabs, this includes the
  // descriptor -- so be careful! For large-order slabs, the descriptor is
  // kmalloc-ed separately and still needs to be freed.
  void *bp = _slab_cache_is_small(slab->parent->order) ? slab : slab->data;
  phys_rra_free_order(slab->parent->allocator, VM_TO_IDM(bp),
                      ilog2(slab->parent->pages));

  // Remove this slab from the parent ll.
  list_del(&slab->ll);
}

void slab_cache_destroy(struct slab_cache *slab_cache) {
#define CLEAR_LIST(field)                                                      \
  while (!list_empty(&slab_cache->field)) {                                    \
    struct slab *slab = list_entry(slab_cache->field.next, struct slab, ll);   \
    _slab_destroy(slab);                                                       \
    if (!_slab_cache_is_small(slab_cache->order)) {                            \
      kfree(slab);                                                             \
    }                                                                          \
  }

  CLEAR_LIST(empty_slabs);
  CLEAR_LIST(partial_slabs);
  CLEAR_LIST(full_slabs);
#undef CLEAR_LIST
}

void slab_allocators_init(void) {
  for (unsigned order = SLAB_MIN_ORDER; order <= SLAB_MAX_ORDER; ++order) {
    slab_cache_init(_slab_allocator_get_cache(order), phys_mem_get_rra(),
                    order);
  }
}

void slab_cache_alloc_slab(struct slab_cache *slab_cache) {
  void *const page =
      phys_rra_alloc_order(slab_cache->allocator, ilog2(slab_cache->pages));
  if (!page) {
    return;
  }
  void *const page_hm = VM_TO_HHDM(page);

  const size_t desc_sz =
      sizeof(struct slab) +
      slab_cache->elements * sizeof(struct slab_freelist_item);
  const size_t element_sz = 1u << slab_cache->order;
  struct slab *slab;
  void *objects_start;
  if (_slab_cache_is_small(slab_cache->order)) {
    // Initialize desc in backing page.
    slab = (struct slab *)page_hm;
    objects_start =
        page_hm + (desc_sz + element_sz - 1) / element_sz * element_sz;
  } else {
    // Allocate descriptor. Note that this will always allocate from the global
    // slab_cache, not the local one if slab_cache->allocator is set to
    // something else (for unit tests).
    slab = (struct slab *)kmalloc(desc_sz);
    objects_start = page_hm;
  }

  if (!slab) {
    phys_rra_free_order(slab_cache->allocator, page, slab_cache->order);
    return;
  }

  list_init(&slab->ll);

  slab->data = objects_start;

  slab->parent = slab_cache;
  slab->allocated = 0;

  // Initialize freelist.
  for (uint8_t i = 0; i < slab_cache->elements; ++i) {
    slab->freelist[i].stack_item = i;
    slab->freelist[i].pos_in_stk = i;
  }

  // Set reference to slab_cache in struct page.
  struct page *const pg_desc = phys_rra_get_page(slab_cache->allocator, page);
  assert(pg_desc);
  pg_desc->context.slab = slab;

  list_add(&slab_cache->empty_slabs, &slab->ll);
}

/**
 * Find a non-full slab in a slab cache. First check the empty list, then the
 * partially-full list. If no slabs exist, then allocate a new slab.
 */
struct slab *_slab_cache_find_nonfull_slab(struct slab_cache *slab_cache) {
  // Look for partially-full slabs first.
  if (!list_empty(&slab_cache->partial_slabs)) {
    return list_entry(slab_cache->partial_slabs.next, struct slab, ll);
  }

  // Look for empty slabs next.
  if (!list_empty(&slab_cache->empty_slabs)) {
    return list_entry(slab_cache->empty_slabs.next, struct slab, ll);
  }

  // Need to allocate a new slab.
  slab_cache_alloc_slab(slab_cache);
  return list_empty(&slab_cache->empty_slabs)
             ? NULL
             : list_entry(slab_cache->empty_slabs.next, struct slab, ll);
}

void *_slab_alloc(struct slab *slab) {
  assert(slab);
  assert(slab->allocated < slab->parent->elements);

  const size_t order_sz = 1u << slab->parent->order;
  void *const obj =
      slab->data + slab->freelist[slab->allocated].stack_item * order_sz;
  ++slab->allocated;
  return obj;
}

void *slab_cache_alloc(struct slab_cache *slab_cache) {
  struct slab *const slab = _slab_cache_find_nonfull_slab(slab_cache);
  if (!slab) {
    return NULL;
  }

  bool is_slab_orig_empty = !slab->allocated;

  // Perform allocation.
  void *obj = _slab_alloc(slab);

  if (slab->allocated == slab->parent->elements) {
    // Move to full list.
    list_del(&slab->ll);
    list_add(&slab_cache->full_slabs, &slab->ll);
  } else if (is_slab_orig_empty) {
    // Move to partially-full list.
    list_del(&slab->ll);
    list_add(&slab_cache->partial_slabs, &slab->ll);
  }

  return obj;
}

void *kmalloc(size_t sz) {
  int order = ilog2ceil(sz);

  if (order < SLAB_MIN_ORDER) {
    order = SLAB_MIN_ORDER;
  }
  struct slab_cache *const slab_cache = _slab_allocator_get_cache(order);
  if (!slab_cache) {
    return NULL;
  }
  return slab_cache_alloc(slab_cache);
}

void _slab_free(struct slab *slab, const void *obj) {
  // Find the position of the object in the freelist.
  // - off: offset in bytes
  // - index: offset in (# of elements)
  // - i: position in stack
  const size_t off = obj - slab->data;
  const unsigned order = slab->parent->order;

  // Assert that the offset is aligned to the size of the object.
  assert(!(off & ((1u << order) - 1)));

  // Find index of the allocated element in the freelist.
  const uint8_t index = off >> order;
  assert(index < slab->parent->elements);
  const uint8_t i = slab->freelist[index].pos_in_stk;

  // Free the element at index 0. Beforehand:
  //
  //          used   |  free
  //     | 0 | 2 | 3 | 4 | 1 | stack_item
  //     | 0 | 4 | 1 | 2 | 3 | pos_in_stk
  //
  //       used  |   free
  //     | 3 | 2 | 0 | 4 | 1 | stack_item
  //     | 2 | 4 | 1 | 0 | 3 | pos_in_stk
  //
  // Afterwards. Note that 0 (the element to be freed) and 3 (the last used
  // element) swap places.
  --slab->allocated;
  slab->freelist[i].stack_item = slab->freelist[slab->allocated].stack_item;
  slab->freelist[slab->allocated].stack_item = index;

  slab->freelist[index].pos_in_stk = slab->allocated;
  slab->freelist[slab->freelist[i].stack_item].pos_in_stk = i;
}

void slab_cache_free(struct slab_cache *slab_cache, struct slab *slab,
                     const void *obj) {
  if (!slab) {
    struct page *pg = phys_rra_get_page(slab_cache->allocator, VM_TO_IDM(obj));
    assert(pg);

    slab = pg->context.slab;
    assert(slab);
    assert(slab->parent == slab_cache);
  }

  bool is_slab_orig_full = slab->allocated == slab_cache->elements;

  // Perform freeing.
  _slab_free(slab, obj);

  if (!slab->allocated) {
    // Move to empty list.
    list_del(&slab->ll);
    list_add(&slab_cache->empty_slabs, &slab->ll);
  } else if (is_slab_orig_full) {
    // Move to partially-full list.
    list_del(&slab->ll);
    list_add(&slab_cache->partial_slabs, &slab->ll);
  }
}

void kfree(const void *obj) {
  struct page *const pg = phys_rra_get_page(phys_mem_get_rra(), VM_TO_IDM(obj));
  assert(pg);

  struct slab *const slab = pg->context.slab;
  assert(slab);

  slab_cache_free(slab->parent, slab, obj);
}
