#include "mem/phys.h"

#include "arch/x86_64/pt.h"
#include "common/libc.h"

#include <assert.h>

static struct _phys_rr_allocator {
  /**
   * Memory bitmap. Will be a HM address, since the new pt
   * won't include a LM identity map.
   */
  char *mem_bitmap;

  /**
   * Physical memory statistics. total_sz == total_pg * PG_SZ,
   * included for convenience.
   */
  size_t total_sz;
  size_t total_pg;
  size_t allocated_pg;

  /**
   * Round-robin scheduling.
   */
  size_t needle;
} _phys_allocator;

/**
 * Helper function for allocating a physical page.
 *
 * Returns true iff the physical page is free.
 */
static bool _phys_page_alloc(void *addr) {
  assert(PG_ALIGNED(addr));
  size_t pg = (size_t)addr >> PG_SZ_BITS;
  if (BM_TEST(_phys_allocator.mem_bitmap, pg)) {
    // Already allocated.
    return false;
  }
  BM_SET(_phys_allocator.mem_bitmap, pg);
  ++_phys_allocator.allocated_pg;
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
  if (!BM_TEST(_phys_allocator.mem_bitmap, pg)) {
    // Not allocated.
    return false;
  }
  BM_CLEAR(_phys_allocator.mem_bitmap, pg);
  --_phys_allocator.allocated_pg;
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

/**
 * Helper function to initialize the round robin allocator,
 * such as zeroing the bitmap.
 *
 * Note that we actually set mem_bitmap to the high-mem-mapped
 * version of addr, because we will not provide the low-mem
 * identity mapping in the new pagetable. (See mem/virt.h).
 *
 * TODO(jlam55555): We are making the subtle assumption here that
 * VM_HM_START is the same as Limine's VM addr start of HHDM.
 * They should be the same value for x86_64, but this is not
 * guaranteed by the Limine spec.
 *
 * TODO(jlam55555): This should really take an allocator as argument,
 * but since we only expect to have a single instance of a physical
 * allocator this is fine for now. We may want to make this change
 * when doing performance testing between multiple physical memory
 * allocators.
 */
static void _phys_rr_allocator_init(void *addr, size_t sz) {
  _phys_allocator.total_sz = sz * PG_SZ;
  _phys_allocator.total_pg = sz;
  _phys_allocator.allocated_pg = 0;
  _phys_allocator.needle = 0;

  // Use HM version of address.
  _phys_allocator.mem_bitmap = (void *)(VM_HM_START | (size_t)addr);

  // Initialize bitmap.
  memset(addr, 0, sz);

  // Mark bitmap pages as allocated. We use the physical
  // address here rather than the HHDM address.
  _phys_region_alloc(addr, PG_COUNT(sz));

  // Shim for now, because the first page shouldn't be usable.
  // TODO(jlam55555): Remove this once we take memory holes into account.
  assert(_phys_page_alloc(NULL));
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

  // Initialize the round robin allocator.
  _phys_rr_allocator_init(mem_bitmap_paddr, bm_sz);

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

// Round-robin page allocator.
void *phys_page_alloc(void) {
  if (_phys_allocator.allocated_pg == _phys_allocator.total_pg) {
    // OOM
    return NULL;
  }

  size_t start_needle = _phys_allocator.needle;
  while (BM_TEST(_phys_allocator.mem_bitmap, _phys_allocator.needle)) {
    if (++_phys_allocator.needle >= _phys_allocator.total_sz) {
      _phys_allocator.needle -= _phys_allocator.total_sz;
    }

    // Sanity check. If this assertion fails, that means we're OOM,
    // but the original OOM check didn't catch this.
    assert(_phys_allocator.needle != start_needle);
  }

  // We could move the needle here so that it points to the next
  // page, or leave it pointing at the last allocated page.
  // The latter option may be useful if pages are used in a LIFO manner.
  void *phys_addr = _phys_allocator.needle * PG_SZ;
  assert(_phys_page_alloc(phys_addr));
  return phys_addr;
}

bool phys_page_free(void *pg) { return _phys_page_free(pg); }

void phys_mem_print_stats(void) {
  printf("\rPhysical page usage %u%%: %lu/%lu pages (%lu/%lu bytes)\r\n",
         _phys_allocator.allocated_pg * 100 / _phys_allocator.total_pg,
         _phys_allocator.allocated_pg, _phys_allocator.total_pg,
         _phys_allocator.allocated_pg * PG_SZ, _phys_allocator.total_sz);
}
