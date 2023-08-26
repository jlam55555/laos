#include "mem/slab.h"

struct slab_cache _slab_caches[SLAB_MAX_ORDER - SLAB_MIN_ORDER + 1];

void _slab_allocator_init(unsigned order, struct slab_cache *slab_cache) {
  slab_cache->order = order;

  // Initialized sentinel nodes.
  slab_cache->empty_slabs->next = NULL;
  slab_cache->partial_slabs->next = NULL;
  slab_cache->full_slabs->next = NULL;
}

struct slab_cache *_slab_allocator_get_cache(unsigned order) {
  if (order < 5 || order > 17) {
    return NULL;
  }
  return &_slab_caches[order - SLAB_MIN_ORDER];
}

void slab_allocators_init(void) {
  for (unsigned order = SLAB_MIN_ORDER; order <= SLAB_MAX_ORDER; ++order) {
    _slab_allocator_init(order, _slab_allocator_get_cache(order));
  }
}

static void *_slab_alloc_small(struct slab_cache *slab_cache) {
  // TODO(jlam55555): Working here.
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

  if (order < 5) {
    order = 5;
  }
  struct slab_cache *slab_cache = _slab_allocator_get_cache(order);
  if (!slab_cache) {
    return NULL;
  }
  return _slab_alloc(slab_cache);
}
