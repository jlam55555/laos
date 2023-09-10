#include "mem/phys.h"

#include "arch/x86_64/pt.h"
#include "common/libc.h"
#include "test/phys_fixture.h"

#include <assert.h>
#include <limine.h>

/**
 * Main physical memory page allocator.
 */
static struct phys_rra _phys_allocator;

/**
 * Helper function for phys_reclaim_bootloader_mem(). Bootloader-reclaimable
 * pages are marked unusable by _phys_region_alloc(). Once we're done with the
 * bootloader-reclaimable memory, we can mark them as usable and update the
 * unusable page count.
 */
static void _phys_region_mark_usable(void *addr, size_t pg_count) {
  for (size_t i = 0; i < pg_count; ++i, addr += PG_SZ) {
    const size_t pg = (size_t)addr >> PG_SZ_BITS;
    assert(!_phys_allocator.mem_bitmap[pg].present);
    assert(_phys_allocator.mem_bitmap[pg].unusable);
    _phys_allocator.mem_bitmap[pg].unusable = false;
  }
  _phys_allocator.unusable_pg -= pg_count;
}

void phys_mem_init(struct limine_memmap_entry *init_mmap, size_t entry_count) {
  // Memory limit is the maximum limit of the physical memory space,
  // including memory holes.
  const size_t mem_limit =
      init_mmap[entry_count - 1].base + init_mmap[entry_count - 1].length;
  assert(PG_ALIGNED(mem_limit));

  // Bitmap size is (dimensional analysis):
  // mem_limit bytes * page/4096bytes * sizeof(struct page) bytes/page.
  const size_t bm_sz = (mem_limit >> PG_SZ_BITS) * sizeof(struct page);

#ifdef DEBUG
  // For diagnostic purposes.
  printf("Maximum physical address=%lx\r\nstruct page array size=%lx\r\n",
         mem_limit, bm_sz);
#endif // DEBUG

  // Allocate physical memory for bitmap in first usable region
  // large enough for it.
  void *mem_bitmap_paddr = NULL;
  for (size_t mmap_entry_i = 0; mmap_entry_i < entry_count; ++mmap_entry_i) {
    struct limine_memmap_entry *mmap_entry = init_mmap + mmap_entry_i;

    // Make sure region is usable. If not usable, we perform some
    // normalization on the region. The Limine spec guarantees that usable
    // regions are page-aligned and usable, whereas neither are guaranteed for
    // non-usable regions.
    //
    // TODO(jlam55555): Should separate the normalization logic out into a
    // separate function. Currently we do it inline because it doesn't require
    // an extra pass, but it's a little confusing to be included here.
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
  // address space. This is not unsolvable, but it may be a little annoying to
  // deal with.
  assert((size_t)mem_bitmap_paddr + bm_sz <= 4 * GiB);

  // Initialize the round robin allocator.
  phys_rra_init(&_phys_allocator, mem_bitmap_paddr, mem_limit, init_mmap,
                entry_count, 0);
}

void phys_reclaim_bootloader_mem(struct limine_memmap_entry *init_mmap,
                                 size_t entry_count) {
  for (size_t mmap_entry_i = 0; mmap_entry_i < entry_count; ++mmap_entry_i) {
    struct limine_memmap_entry *mmap_entry = init_mmap + mmap_entry_i;
    if (mmap_entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE) {
      _phys_region_mark_usable((void *)mmap_entry->base,
                               PG_COUNT(mmap_entry->length));
    }
  }
}

void *phys_alloc_page(void) {
  const void *rv = phys_rra_alloc_order(&_phys_allocator, 0);
  if (rv) {
    return VM_TO_HHDM(rv);
  }
  return NULL;
}

void phys_free_page(const void *pg) {
  phys_rra_free_order(&_phys_allocator, VM_TO_IDM(pg), 0);
}

struct phys_rra *phys_mem_get_rra(void) {
  return &_phys_allocator;
}
/**
 * Allocates a physical page. Errors if the page is unusable (e.g., hole
 * memory). Returns true iff the physical page is free.
 */
bool _phys_rra_alloc(struct phys_rra *rra, const void *addr) {
  assert(addr);
  const size_t pg = (size_t)(addr - rra->phys_offset) >> PG_SZ_BITS;
  assert(pg < rra->total_pg);
  assert(!rra->mem_bitmap[pg].unusable);
  if (rra->mem_bitmap[pg].present) {
    // Already allocated.
    return false;
  }
  rra->mem_bitmap[pg].present = true;
  ++rra->allocated_pg;
  return true;
}

/**
 * Frees a physical page. Errors if the page is unusable (e.g., hole memory).
 * Returns true iff the physical page is allocated.
 */
bool _phys_rra_free(struct phys_rra *rra, const void *addr) {
  assert(addr);
  assert(PG_ALIGNED(addr));
  const size_t pg = (size_t)(addr - rra->phys_offset) >> PG_SZ_BITS;
  assert(!rra->mem_bitmap[pg].unusable);
  if (!rra->mem_bitmap[pg].present) {
    // Not allocated.
    return false;
  }
  rra->mem_bitmap[pg].present = false;
  --rra->allocated_pg;
  return true;
}

/**
 * Helper function to force allocation of all physical page regions. This is
 * only used during initialization and all the pages in this region are assumed
 * to be free.
 *
 * For unusable pages, do not actually allocate -- we simply mark it as
 * unusable. This is because we don't want to include unusable regions in the
 * bookkeeping for allocatable pages.
 *
 * If this assumption is false, we error.
 */
static void _phys_rra_alloc_region(struct phys_rra *rra, void *addr,
                                   size_t pg_count, bool is_unusable) {
  for (size_t i = 0; i < pg_count; ++i, addr += PG_SZ) {
    if (is_unusable) {
      size_t pg = (size_t)addr >> PG_SZ_BITS;
      assert(!rra->mem_bitmap[pg].present);
      rra->mem_bitmap[pg].unusable = true;
    } else {
      assert(_phys_rra_alloc(rra, addr));
    }
  }
  // Adjust unusable page counts.
  if (is_unusable) {
    rra->unusable_pg += pg_count;
  }
}

/**
 * Note that we actually set mem_bitmap to the high-mem-mapped version of addr,
 * because we will not provide the low-mem identity mapping in the new
 * pagetable. (See mem/virt.h).
 *
 * TODO(jlam55555): We are making the subtle assumption here that VM_HM_START is
 * the same as Limine's VM addr start of HHDM. They should be the same value for
 * x86_64, but this is not guaranteed by the Limine spec. We can use the Limine
 * HHDM feature to get the start of the Limine HHDM LIMINE_HHDM, and assert that
 * LIMINE_HHDM <= VM_HM_START.
 */
void phys_rra_init(struct phys_rra *rra, void *addr, size_t mem_limit,
                   struct limine_memmap_entry *init_mmap, size_t entry_count,
                   void *phys_offset) {
  rra->total_sz = mem_limit;
  rra->total_pg = mem_limit >> PG_SZ_BITS;
  rra->allocated_pg = 0;
  rra->unusable_pg = 0;
  rra->needle = 0;
  rra->phys_offset = phys_offset;

  // Use HM version of address.
  rra->mem_bitmap = VM_TO_HHDM(addr);

  // Initialize bitmap. bm_sz = "bitmap size"
  const size_t bm_sz = rra->total_pg * sizeof(struct page);
  memset(rra->mem_bitmap, 0, bm_sz);

  // Mark bitmap pages as allocated. This should only be done for the main rra,
  // as other bootstrapped rras (e.g., for testing) will not reference
  // themselves.
  if (rra == &_phys_allocator) {
    _phys_rra_alloc_region(rra, VM_TO_IDM(rra->mem_bitmap), PG_COUNT(bm_sz),
                           false);
  }

  // Mark unusable regions in the bitmap.
  void *prev_end = NULL;
  for (size_t mmap_entry_i = 0; mmap_entry_i < entry_count; ++mmap_entry_i) {
    struct limine_memmap_entry *mmap_entry = init_mmap + mmap_entry_i;

    // Memory hole detected. Mark it as unusable memory.
    if ((size_t)prev_end != mmap_entry->base) {
      _phys_rra_alloc_region(rra, VM_TO_IDM(prev_end),
                             PG_COUNT((void *)mmap_entry->base - prev_end),
                             true);
    }
    prev_end = (void *)(mmap_entry->base + mmap_entry->length);

    // Mark unusable memory regions. (This includes bootloader-reclaimable
    // memory regions, which will be freed/marked usable later on.)
    if (mmap_entry->type != LIMINE_MEMMAP_USABLE) {
      _phys_rra_alloc_region(rra, VM_TO_IDM(mmap_entry->base),
                             PG_COUNT(mmap_entry->length), true);
    }
  }
}

void phys_mem_print_stats(void) {
  printf("\rPhysical page usage %u%%: %lu/%lu pages (%lu/%lu bytes)\r\n",
         _phys_allocator.allocated_pg * 100 /
             (_phys_allocator.total_pg - _phys_allocator.unusable_pg),
         _phys_allocator.allocated_pg,
         (_phys_allocator.total_pg - _phys_allocator.unusable_pg),
         _phys_allocator.allocated_pg << PG_SZ_BITS,
         _phys_allocator.total_sz -
             (_phys_allocator.unusable_pg << PG_SZ_BITS));
}

/**
 * Helper function to check if a continuous region of size 2^order can be
 * allocated at the current needle position in `rra`.
 */
bool _phys_rra_can_alloc_order_at(const struct phys_rra *rra, size_t order) {
  const size_t pages = 1u << order;

  // Not enough contiguous pages left after the needle.
  if (rra->needle + pages > rra->total_pg) {
    return false;
  }

  const struct page *entry = rra->mem_bitmap + rra->needle;
  for (size_t i = 0; i < pages; ++i, ++entry) {
    if (entry->present || entry->unusable) {
      return false;
    }
  }
  return true;
}

void *phys_rra_alloc_order(struct phys_rra *rra, unsigned order) {
  if (rra->allocated_pg == rra->total_pg) {
    // OOM
    return NULL;
  }

  const size_t start_needle = rra->needle;
  const size_t pages = 1u << order;
  while (!_phys_rra_can_alloc_order_at(rra, order)) {
    if (++rra->needle >= rra->total_pg) {
      rra->needle -= rra->total_pg;
    }

    // Could not allocate at this size.
    if (rra->needle == start_needle) {
      return NULL;
    }
  }

  // Don't move to the end of the allocated region. Although we could. This
  // behavior is nicer if we often free a page right after allocating it.
  void *phys_addr = (void *)((size_t)rra->needle << PG_SZ_BITS);
  phys_addr += (uint64_t)rra->phys_offset;
  _phys_rra_alloc_region(rra, phys_addr, pages, false);
  return phys_addr;
}

void phys_rra_free_order(struct phys_rra *rra, const void *pg, unsigned order) {
  const size_t pages = 1u << order;
  for (unsigned i = 0; i < pages; ++i, pg += PG_SZ) {
    assert(_phys_rra_free(rra, pg));
  }
}

struct page *phys_rra_get_page(struct phys_rra *rra, const void *pg) {
  if (pg) {
    pg -= (uint64_t)rra->phys_offset;
  }
  return &rra->mem_bitmap[(size_t)pg >> PG_SZ_BITS];
}
