#include "test/mem_harness.h"

#include "mem/phys.h" // for struct phys_rra
#include "mem/slab.h" // for kmalloc
#include "mem/vm.h"   // for VM_TO_IDM

#include <assert.h> // for assert

struct phys_rra *phys_fixture_create_rra(void) {
  // Allocate a backing buffer for the page array.
  void *page_array_bb = kmalloc(16 * sizeof(struct page));
  assert(page_array_bb);

  // Allocate a backing buffer for the actual page data.
  void *bb = phys_rra_alloc_order(phys_mem_get_rra(), 4);
  assert(bb);

  const size_t length = 16 * PG_SZ;
  struct limine_memmap_entry mmap_entries[] = {
      {.base = 0x0, .length = length, .type = LIMINE_MEMMAP_USABLE},
  };
  const size_t entry_count = sizeof(mmap_entries) / sizeof(mmap_entries[0]);

  struct phys_rra *rra = kmalloc(sizeof(struct phys_rra));
  phys_rra_init(rra, VM_TO_IDM(page_array_bb), length, mmap_entries,
                entry_count, bb);

  return rra;
}

void phys_fixture_destroy_rra(struct phys_rra *rra) {
  assert(rra);

  // Deallocate page array backing buffer.
  kfree(rra->mem_bitmap);

  // Deallocate backing buffer.
  phys_rra_free_order(phys_mem_get_rra(), rra->phys_offset, 4);

  // Deallocate RR page allocator descriptor.
  kfree(rra);
}

struct slab_cache *slab_fixture_create_slab_cache(unsigned order) {
  struct phys_rra *rra;
  assert(rra = phys_fixture_create_rra());

  struct slab_cache *slab_cache = kmalloc(sizeof(struct slab_cache));
  slab_cache_init(slab_cache, rra, order);

  return slab_cache;
}

void slab_fixture_destroy_slab_cache(struct slab_cache *slab_cache) {
  struct phys_rra *rra = slab_cache->allocator;
  slab_cache_destroy(slab_cache);
  phys_fixture_destroy_rra(rra);
  kfree(slab_cache);
}
