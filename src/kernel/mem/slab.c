#include "mem/slab.h"

#include "common/libc.h"
#include "mem/phys.h"

#include <assert.h>

struct slab_cache _slab_caches[SLAB_MAX_ORDER - SLAB_MIN_ORDER + 1];

static bool _slab_cache_is_small(unsigned order) {
  return order <= SLAB_SMALL_MAX_ORDER;
}

/**
 * Requirements for number of elements per slab of size S (usually S == PG_SZ,
 * but S must be larger if 2^M > PG_SZ):
 * - For small object slabs of order M:
 *   - sizeof(struct slab) + N + N*2^M <= S.
 *   - Note that the objects need to be 2^M aligned as well.
 * - For large object slabs of order M:
 *   - N*2^M <= S
 *   - sizeof(struct slab) + N <= 2^SLAB_SMALL_MAX_ORDER (This requirement is so
 *     that we can allocate the slab descriptor using a small-object slab
 *     allocator. Technically, the descriptor only has to fit on a lower-order
 *     slab allocator but let's try to keep descriptors small-ish.)
 */
void _slab_allocator_init(unsigned order, struct slab_cache *slab_cache) {
  slab_cache->order = order;
  slab_cache->empty_slabs = NULL;
  slab_cache->partial_slabs = NULL;
  slab_cache->full_slabs = NULL;

  size_t element_size = 1u << order;
  size_t desc_size, slab_size;
  if (_slab_cache_is_small(order)) {
    slab_cache->pages = 1;
    slab_cache->elements =
        (PG_SZ - sizeof(struct slab)) / (element_size + 1) - 1;

    desc_size = sizeof(struct slab) + slab_cache->elements;
    slab_size = (element_size * (desc_size + element_size - 1) / element_size) +
                element_size * slab_cache->elements;

    // Descriptor + freelist + objects must fit on one page.
    assert(slab_size <= PG_SZ);
  } else {
    slab_cache->pages = order >= PG_SZ_BITS ? PG_COUNT(1u << order) : 1;
    slab_cache->elements = (slab_cache->pages << PG_SZ_BITS) >> order;

    desc_size = sizeof(struct slab) + slab_cache->elements;
    slab_size = element_size * slab_cache->elements;
    assert(slab_size == PG_SZ * slab_cache->pages);

    // Descriptor + freelist must fit in the descriptor of a lower-level
    // freelist.
    assert(desc_size <= (1u << SLAB_SMALL_MAX_ORDER));
  }

  printf("slaballoc: order=%u pages=%u elements=%u small=%u "
         "desc_size=%lu slab_size=%lu\r\n",
         slab_cache->order, slab_cache->pages, slab_cache->elements,
         _slab_cache_is_small(order), desc_size, slab_size);
}

struct slab_cache *_slab_allocator_get_cache(unsigned order) {
  if (order < SLAB_MIN_ORDER || order > SLAB_MAX_ORDER) {
    return NULL;
  }
  return &_slab_caches[order - SLAB_MIN_ORDER];
}

void slab_allocators_init(void) {
  for (unsigned order = SLAB_MIN_ORDER; order <= SLAB_MAX_ORDER; ++order) {
    _slab_allocator_init(order, _slab_allocator_get_cache(order));
  }
}

struct slab *_slab_cache_find_nonfull_slab(struct slab_cache *slab_cache) {
  // Look for partially-full slabs first.
  if (slab_cache->partial_slabs) {
    return slab_cache->partial_slabs;
  }

  // Look for empty slabs next.
  if (slab_cache->empty_slabs) {
    return slab_cache->empty_slabs;
  }

  // Need to allocate a new slab.
  return NULL;
}

static void *_slab_alloc_small(struct slab_cache *slab_cache) {
  // TODO(jlam55555): Working here.
  struct slab *slab = _slab_cache_find_nonfull_slab(slab_cache);
  return NULL;
}

static void *_slab_alloc_large(struct slab_cache *slab_cache) {
  // TODO(jlam55555): Working here.
  return NULL;
}

static void *_slab_alloc(struct slab_cache *slab_cache) {
  return slab_cache->order <= SLAB_SMALL_MAX_ORDER
             ? _slab_alloc_small(slab_cache)
             : _slab_alloc_large(slab_cache);
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
  struct slab_cache *slab_cache = _slab_allocator_get_cache(order);
  if (!slab_cache) {
    return NULL;
  }
  return _slab_alloc(slab_cache);
}
