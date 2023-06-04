#include "mem/phys.h"

#include "common/libc.h"

#include <assert.h>

/**
 * Memory bitmap. Will be initialized in phys_mem_init().
 * This will also need to be mapped into virtual memory
 * by phys_mem_init().
 *
 * TODO(jlam55555): figure out the gymnastics there.
 */
static char *_mem_bitmap;

/**
 * Physical memory statistics.
 */
static size_t _total_pg;
static size_t _allocated_pg;

/**
 * Helper function to initialize the mem_bitmap.
 */
static void _setup_page_bm(void *addr, size_t sz) {
  _total_pg = sz << 3;
  _allocated_pg = 0;
  memset(_mem_bitmap = addr, 0, sz);
}

/**
 * Helper function for allocating a physical page.
 *
 * Returns true iff the physical page is free.
 */
static bool _phys_page_alloc(void *addr) {
  assert(PG_ALIGNED(addr));
  size_t pg = (size_t)addr >> PG_SZ_BITS;
  if (BM_TEST(_mem_bitmap, pg)) {
    // Already allocated.
    return false;
  }
  BM_SET(_mem_bitmap, pg);
  ++_allocated_pg;
  return true;
}

/**
 * Helper function for freeing a physical page.
 *
 * Returns true iff the physical page is allocated.
 *
 * TODO(jlam55555): Alert if the physical page is
 * reserved or lies in a memory hole.
 */
static bool _phys_page_free(void *addr) {
  assert(PG_ALIGNED(addr));
  size_t pg = (size_t)addr >> PG_SZ_BITS;
  if (!BM_TEST(_mem_bitmap, pg)) {
    // Not allocated.
    return false;
  }
  BM_CLEAR(_mem_bitmap, pg);
  --_allocated_pg;
  return true;
}

/**
 * Helper function to force allocation of all physical page
 * regions. This is only used during initialization and all
 * the pages in this region are assumed to be free.
 *
 * If this assumption is false, we error.
 */
static void _phys_region_alloc(void *addr, size_t pg_count) {
  for (size_t i = 0; i < pg_count; ++i, addr += PG_SZ) {
    assert(_phys_page_alloc(addr));
  }
}

void phys_mem_init(struct limine_memmap_entry *init_mmap, size_t entry_count) {
  // TODO(jlam55555): Detect memory limit from init mmap.
  // TODO(jlam55555): Determine what to do with memory holes.
  size_t mem_limit = (size_t)4 * 1024 * 1024 * 1024;
  assert(PG_ALIGNED(mem_limit));

  // Bitmap size is (dimensional analysis):
  // mem_limit bytes * page/4096bytes * 1bit/1page * 1byte/8bits.
  size_t bm_sz = mem_limit >> 15;

  // Allocate physical memory for bitmap in first usable region
  // large enough for it.
  void *mem_bitmap_paddr = NULL;
  for (size_t mmap_entry_i = 0; mmap_entry_i < entry_count; ++mmap_entry_i) {
    struct limine_memmap_entry *mmap_entry = init_mmap + mmap_entry_i;

    // Make sure region is usable.
    if (mmap_entry->type != LIMINE_MEMMAP_USABLE) {
      continue;
    }

    // Normalize region; make sure it's page-aligned.
    void *mmap_region_start = PG_CEIL(mmap_entry->base);
    void *mmap_region_end = PG_FLOOR(mmap_entry->base + mmap_entry->length);

    // Found a region large enough.
    if ((size_t)(mmap_region_end - mmap_region_start) >= bm_sz) {
      mem_bitmap_paddr = mmap_region_start;
      break;
    }
  }

  // No usable regions large enough for memory bitmap.
  assert(mem_bitmap_paddr);

  // Make sure this usable region lies within the identity-mapped
  // region provided by Limine (first four GiB), otherwise our virtual
  // address will not be correct.
  // TODO(jlam55555): This is hardcoded for now based on the Limine
  // protocol, it would be better if we could check this programmatically.
  assert((size_t)mem_bitmap_paddr + bm_sz <= (size_t)4 * 1024 * 1024 * 1024);

  // Initialize the bitmap.
  _setup_page_bm(mem_bitmap_paddr, bm_sz);

  // Mark bitmap pages as allocated.
  _phys_region_alloc(mem_bitmap_paddr, PG_COUNT(bm_sz));

  // Mark other pages from init_mmap as allocated.
  // TODO(jlam55555): Need to correctly handle overlapping (unusable) regions
  // and memory holes. Overlapping unusable regions are allowed by the Limine
  // spec but would fail the assertion in _phys_region_alloc(). Memory holes
  // should be marked as allocated/unusable, but not count towards the used
  // total memory statistics.
  for (size_t mmap_entry_i = 0; mmap_entry_i < entry_count; ++mmap_entry_i) {
    struct limine_memmap_entry *mmap_entry = init_mmap + mmap_entry_i;
    if (mmap_entry->type == LIMINE_MEMMAP_USABLE) {
      continue;
    }
    // A bit of a paranoid computation, in case regions are not page-aligned.
    _phys_region_alloc(PG_FLOOR(mmap_entry->base),
                       PG_COUNT(mmap_entry->base + mmap_entry->length -
                                (size_t)PG_FLOOR(mmap_entry->base)));
  }

  // TODO(jlam55555): Remove this.
  phys_mem_print_stats();
}
void *phys_page_alloc(void) {}
bool phys_page_free(void *pg) { return false; }
void phys_mem_print_stats(void) {
  printf("Physical page usage %u%%: %lu/%lu pages (%lu/%lu bytes)\r\n",
         (_allocated_pg * 100 / _total_pg) / 100, _allocated_pg, _total_pg,
         _allocated_pg * PG_SZ, _total_pg * PG_SZ);
}
