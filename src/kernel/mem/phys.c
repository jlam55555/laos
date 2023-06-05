#include "mem/phys.h"

#include "arch/x86_64/pt.h"
#include "common/libc.h"

#include <assert.h>

/**
 * State variable singleton struct for the physical memory allocator.
 */
static struct _phys_rr_allocator {
  /**
   * Memory bitmap. Will be a HM address, since the new pt won't include a LM
   * identity map.
   */
  char *mem_bitmap;

  /**
   * Physical memory statistics. total_sz == total_pg * PG_SZ, included for
   * convenience. Total size is the end of the physical address space.
   *
   * Note that hole_pg is counted in the total_pg but not the allocated_pg. That
   * is, total_pg = allocated_pg + hole_pg + <free pg>. This definition may
   * change in the future if a more convenient definition arises.
   */
  size_t total_sz;
  size_t total_pg;
  size_t allocated_pg;
  size_t hole_pg;

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
  assert(pg < _phys_allocator.total_pg);
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
 * TODO(jlam55555): Alert if the physical page is reserved or lies in a memory
 * hole. We will need a larger bitmap to store this information.
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
 * Helper function to force allocation of all physical page regions. This is
 * only used during initialization and all the pages in this region are assumed
 * to be free.
 *
 * If this assumption is false, we error.
 */
static void _phys_region_alloc(void *addr, size_t pg_count, bool is_hole) {
  for (size_t i = 0; i < pg_count; ++i, addr += PG_SZ) {
    assert(_phys_page_alloc(addr));
  }
  // Manually adjust hole page counts.
  if (is_hole) {
    _phys_allocator.allocated_pg -= pg_count;
    _phys_allocator.hole_pg += pg_count;
  }
}

/**
 * Helper function to initialize the round robin allocator, such as zeroing the
 * bitmap.
 *
 * Note that we actually set mem_bitmap to the high-mem-mapped version of addr,
 * because we will not provide the low-mem identity mapping in the new
 * pagetable. (See mem/virt.h).
 *
 * TODO(jlam55555): We are making the subtle assumption here that VM_HM_START is
 * the same as Limine's VM addr start of HHDM. They should be the same value for
 * x86_64, but this is not guaranteed by the Limine spec. We can use the Limine
 * HHDM feature to get the start of the Limine HHDM LIMINE_HHDM, and assert that
 * LIMINE_HHDM <= VM_HM_START.
 *
 * TODO(jlam55555): This should really take an allocator as argument, but since
 * we only expect to have a single instance of a physical allocator this is fine
 * for now. We may want to make this change when doing performance testing
 * between multiple physical memory allocators.
 */
static void _phys_rr_allocator_init(void *addr, size_t mem_limit,
                                    struct limine_memmap_entry *init_mmap,
                                    size_t entry_count) {
  _phys_allocator.total_sz = mem_limit;
  _phys_allocator.total_pg = mem_limit >> PG_SZ_BITS;
  _phys_allocator.allocated_pg = 0;
  _phys_allocator.needle = 0;

  // Use HM version of address.
  _phys_allocator.mem_bitmap = (void *)(VM_HM_START | (size_t)addr);

  // Initialize bitmap.
  size_t bm_sz = _phys_allocator.total_pg >> 3;
  memset(addr, 0, bm_sz);

  // Mark bitmap pages as allocated. We use the physical address here rather
  // than the HHDM address.
  _phys_region_alloc(addr, PG_COUNT(bm_sz), false);

  // Mark allocated regions in the bitmap, including holes.
  void *prev_end = NULL;
  for (size_t mmap_entry_i = 0; mmap_entry_i < entry_count; ++mmap_entry_i) {
    struct limine_memmap_entry *mmap_entry = init_mmap + mmap_entry_i;

    // Memory hole detected. Mark it as a hole page.
    if ((size_t)prev_end != mmap_entry->base) {
      _phys_region_alloc(prev_end,
                         PG_COUNT((void *)mmap_entry->base - prev_end), true);
    }
    prev_end = (void *)(mmap_entry->base + mmap_entry->length);

    // Mark non-usable regions as allocated.
    if (mmap_entry->type != LIMINE_MEMMAP_USABLE) {
      _phys_region_alloc((void *)mmap_entry->base, PG_COUNT(mmap_entry->length),
                         false);
    }
  }
}

void phys_mem_init(struct limine_memmap_entry *init_mmap, size_t entry_count) {
  // Memory limit is the maximum limit of the physical memory space,
  // including memory holes.
  size_t mem_limit =
      init_mmap[entry_count - 1].base + init_mmap[entry_count - 1].length;
  assert(PG_ALIGNED(mem_limit));

  // Bitmap size is (dimensional analysis):
  // mem_limit bytes * page/4096bytes * 1bit/1page * 1byte/8bits.
  size_t bm_sz = mem_limit >> 15;

  // Allocate physical memory for bitmap in first usable region
  // large enough for it.
  void *mem_bitmap_paddr = NULL;
  for (size_t mmap_entry_i = 0; mmap_entry_i < entry_count; ++mmap_entry_i) {
    struct limine_memmap_entry *mmap_entry = init_mmap + mmap_entry_i;

    // Make sure region is usable. If not usable, we perform some
    // normalization on the region.
    if (mmap_entry->type != LIMINE_MEMMAP_USABLE) {
      // Normalize region; make sure it's page-aligned. The Limine spec dictates
      // that entries that are not usable or bootloader-reclaimable may be
      // non-page-aligned and overlapping.
      void *end = PG_CEIL(mmap_entry->base + mmap_entry->length);
      mmap_entry->base = (size_t)PG_FLOOR(mmap_entry->base);
      mmap_entry->length = (size_t)(end - mmap_entry->base);

      // Normalize region: if region overlaps with the previous one, then
      // shorten the previous one to eliminate the overlap. This simplifies the
      // page table logic.
      if (mmap_entry_i && (mmap_entry - 1)->base + (mmap_entry - 1)->length >
                              mmap_entry->base) {
        (mmap_entry - 1)->length = mmap_entry->base - (mmap_entry - 1)->base;
      }
      continue;
    }

    // Found a region large enough.
    if (!mem_bitmap_paddr && mmap_entry->length >= bm_sz) {
      mem_bitmap_paddr = (void *)mmap_entry->base;
    }
  }

  // No usable regions large enough for memory bitmap.
  assert(mem_bitmap_paddr);

  // Make sure this usable region lies within the identity-mapped region
  // provided by Limine (first four GiB), otherwise we will fall outside the VM
  // address space.
  assert((size_t)mem_bitmap_paddr + bm_sz <= 4 * GiB);

  // Initialize the round robin allocator.
  _phys_rr_allocator_init(mem_bitmap_paddr, mem_limit, init_mmap, entry_count);
}

/**
 * Round-robin page allocator.
 */
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

    // Sanity check. If this assertion fails, that means we're OOM, but the
    // original OOM check didn't catch this.
    assert(_phys_allocator.needle != start_needle);
  }

  // We could move the needle here so that it points to the next page, or leave
  // it pointing at the last allocated page. The latter option may be useful if
  // pages are used in a LIFO manner.
  void *phys_addr = (void *)(_phys_allocator.needle * PG_SZ);
  assert(_phys_page_alloc(phys_addr));
  return phys_addr;
}

/**
 * Free physical page.
 */
void phys_page_free(void *pg) { assert(_phys_page_free(pg)); }

void phys_mem_print_stats(void) {
  printf("\rPhysical page usage %u%%: %lu/%lu pages (%lu/%lu bytes)\r\n",
         _phys_allocator.allocated_pg * 100 /
             (_phys_allocator.total_pg - _phys_allocator.hole_pg),
         _phys_allocator.allocated_pg,
         (_phys_allocator.total_pg - _phys_allocator.hole_pg),
         _phys_allocator.allocated_pg << PG_SZ_BITS,
         _phys_allocator.total_sz - (_phys_allocator.hole_pg << PG_SZ_BITS));
}
