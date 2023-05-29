#include "mem/phys.h"

#include <assert.h>

/**
 * Memory bitmap. Will be initialized in phys_mem_init().
 * This will also need to be mapped into virtual memory
 * by phys_mem_init().
 *
 * TODO(jlam55555): figure out the gymnastics there.
 */
static char *mem_bitmap;

/**
 * Helper function for allocating a physical page. Returns NULL
 * if the requested address is no good, otherwise returns the
 * requested address.
 */
static void *_phys_page_alloc(void *addr) {
  // TODO(jlam55555): Implement this.
}

void phys_mem_init(struct limine_memmap_entry *init_mmap, size_t entry_count) {
  // TODO(jlam55555): Detect memory limit from init mmap.
  size_t mem_limit = 4 * 1024 * 1024 * 1024;
  assert(PG_ALIGNED(mem_limit));

  // Bitmap size is mem_limit * page/4096bytes * byte/8bits.
  // (^ Dimensional analysis)
  size_t bm_sz = mem_limit >> 15;

  // Allocate physical memory for bitmap. Make sure it doesn't
  // overlap any reserved regions in the init_mmap.
  void *mem_bitmap_paddr /* = some_nonoverlapping_addr */;
  mem_bitmap_paddr = _phys_page_alloc(mem_bitmap_paddr);
  assert(mem_bitmap_paddr);

  // Allocate virtual memory without allocating physical memory.
  mem_bitmap = (char *)virt_mem_map_noalloc(mem_bitmap_paddr, PG_COUNT(bm_sz));

  // Allocate all the entries from init_mmap.
  // TODO(jlam55555)
}
void *phys_page_alloc(void) {}
bool phys_page_free(void *pg) { return false; }
void phys_mem_print_stats(void) {}
