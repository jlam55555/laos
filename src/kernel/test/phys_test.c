#include "arch/x86_64/pt.h"
#include "limine.h"
#include "mem/phys.h"

#include "mem/slab.h"
#include "test/test.h"

/**
 * Make sure that we can alloc pages using the main allocator without problem.
 * This shouldn't fail unless we're severely memory-constrained.
 */
DEFINE_TEST(phys, alloc_free_page) {
  void *pg = phys_alloc_page();
  TEST_ASSERT(pg);
  phys_free_page(pg);
}

DEFINE_TEST(phys, alloc_free_page_multiple) {
  void *pg1 = phys_alloc_page();
  TEST_ASSERT(pg1);

  void *pg2 = phys_alloc_page();
  TEST_ASSERT(pg2);

  phys_free_page(pg1);

  void *pg3 = phys_alloc_page();
  TEST_ASSERT(pg3);

  phys_free_page(pg2);
  phys_free_page(pg3);
}

/**
 * `_phys_test_create_rra()` and `_phys_test_destroy_rra()` form a test fixture
 * for a physical allocator. This depends both on the main physical allocator
 * and the slab allocator. Note that there is no underlying memory (the only
 * underlying memory is for the allocator descriptor and the page array), so
 * writing to the pages "allocated" by this test allocator is not safe.
 *
 * TODO(jlam55555): Have a more official test fixture (setup/teardown)
 * framework.
 */
struct phys_rra *_phys_test_create_rra(void) {
  // Allocate a backing buffer for the page array.
  void *page_array_bb = phys_alloc_page();
  assert(page_array_bb);

  const size_t base = PG_SZ;
  const size_t length = 16 * PG_SZ;
  struct limine_memmap_entry mmap_entries[] = {
      {.base = base, .length = length, .type = LIMINE_MEMMAP_USABLE},
  };
  const size_t entry_count = sizeof(mmap_entries) / sizeof(mmap_entries[0]);

  struct phys_rra *rra = kmalloc(sizeof(struct phys_rra));
  phys_rra_init(rra, VM_TO_IDM(page_array_bb), base + length, mmap_entries,
                entry_count);

  return rra;
}

void _phys_test_destroy_rra(struct phys_rra *rra) {
  assert(rra);

  // Deallocate page array backing buffer.
  phys_free_page(rra->mem_bitmap);

  // Deallocate RR page allocator descriptor.
  kfree(rra);
}

/**
 * Check if two regions overlap. See https://stackoverflow.com/a/3269471.
 */
bool _phys_test_overlaps(void *start1, size_t len1, void *start2, size_t len2) {
  return start1 < (start2 + len2) && start2 < (start1 + len1);
}
#define TEST_ASSERT_NOVERLAP2(a, a_sz, b, b_sz)                                \
  TEST_ASSERT(!_phys_test_overlaps(a, a_sz, b, b_sz))
#define TEST_ASSERT_NOVERLAP3(a, a_sz, b, b_sz, c, c_sz)                       \
  TEST_ASSERT_NOVERLAP2(a, a_sz, b, b_sz);                                     \
  TEST_ASSERT_NOVERLAP2(a, a_sz, c, c_sz);                                     \
  TEST_ASSERT_NOVERLAP2(b, b_sz, c, c_sz)
#define TEST_ASSERT_NOVERLAP4(a, a_sz, b, b_sz, c, c_sz, d, d_sz)              \
  TEST_ASSERT_NOVERLAP3(a, a_sz, b, b_sz, c, c_sz);                            \
  TEST_ASSERT_NOVERLAP2(a, a_sz, d, d_sz);                                     \
  TEST_ASSERT_NOVERLAP2(b, b_sz, d, d_sz);                                     \
  TEST_ASSERT_NOVERLAP2(c, c_sz, d, d_sz)

DEFINE_TEST(phys, rra_alloc) {
  struct phys_rra *rra = _phys_test_create_rra();
  TEST_ASSERT(rra);

  void *pg = phys_rra_alloc_order(rra, 0);
  TEST_ASSERT(pg);
  phys_rra_free_order(rra, pg, 0);

  _phys_test_destroy_rra(rra);
}

DEFINE_TEST(phys, rra_alloc_multiple) {
  struct phys_rra *rra = _phys_test_create_rra();
  TEST_ASSERT(rra);

  void *pg1 = phys_rra_alloc_order(rra, 0);
  TEST_ASSERT(pg1);

  void *pg2 = phys_rra_alloc_order(rra, 0);
  TEST_ASSERT(pg2);

  TEST_ASSERT_NOVERLAP2(pg1, PG_SZ, pg2, PG_SZ);

  phys_rra_free_order(rra, pg1, 0);

  void *pg3 = phys_rra_alloc_order(rra, 0);
  TEST_ASSERT(pg3);

  TEST_ASSERT_NOVERLAP2(pg2, PG_SZ, pg3, PG_SZ);

  phys_rra_free_order(rra, pg2, 0);
  phys_rra_free_order(rra, pg3, 0);

  _phys_test_destroy_rra(rra);
}

DEFINE_TEST(phys, rra_alloc_nonzero_order) {
  struct phys_rra *rra = _phys_test_create_rra();
  TEST_ASSERT(rra);

  void *pg1 = phys_rra_alloc_order(rra, 1);
  TEST_ASSERT(pg1);

  void *pg2 = phys_rra_alloc_order(rra, 2);
  TEST_ASSERT(pg2);

  void *pg3 = phys_rra_alloc_order(rra, 3);
  TEST_ASSERT(pg3);

  void *pg4 = phys_rra_alloc_order(rra, 0);
  TEST_ASSERT(pg4);

  phys_rra_free_order(rra, pg1, 1);
  phys_rra_free_order(rra, pg2, 2);
  phys_rra_free_order(rra, pg3, 3);
  phys_rra_free_order(rra, pg4, 0);

  TEST_ASSERT_NOVERLAP4(pg1, 2 * PG_SZ, pg2, 4 * PG_SZ, pg3, 8 * PG_SZ, pg4,
                        PG_SZ);

  _phys_test_destroy_rra(rra);
}

#undef TEST_ASSERT_NOVERLAP2
#undef TEST_ASSERT_NOVERLAP3
#undef TEST_ASSERT_NOVERLAP4

DEFINE_TEST(phys, rra_alloc_oom_order0) {
  struct phys_rra *rra = _phys_test_create_rra();
  TEST_ASSERT(rra);

  void *pg;
  for (size_t i = 0; i < 16; ++i) {
    pg = phys_rra_alloc_order(rra, 0);
    TEST_ASSERT(pg);
  }

  // Try to allocate past full. This should fail.
  TEST_ASSERT(!phys_rra_alloc_order(rra, 0));

  // Free one element, see that we can allocate one more element.
  phys_rra_free_order(rra, pg, 0);
  TEST_ASSERT(phys_rra_alloc_order(rra, 0));
  TEST_ASSERT(!phys_rra_alloc_order(rra, 0));

  // Ok not to clean up here, since the rra will be destroyed.
  _phys_test_destroy_rra(rra);
}

DEFINE_TEST(phys, rra_alloc_oom_order1) {
  struct phys_rra *rra = _phys_test_create_rra();
  TEST_ASSERT(rra);

  const size_t order = 1;

  void *pg;
  for (size_t i = 0; i < 8; ++i) {
    pg = phys_rra_alloc_order(rra, order);
    TEST_ASSERT(pg);
  }

  // Try to allocate past full. This should fail.
  TEST_ASSERT(!phys_rra_alloc_order(rra, order));

  // Free one element, see that we can allocate one more element.
  phys_rra_free_order(rra, pg, order);
  TEST_ASSERT(phys_rra_alloc_order(rra, order));
  TEST_ASSERT(!phys_rra_alloc_order(rra, order));

  // Ok not to clean up here, since the rra will be destroyed.
  _phys_test_destroy_rra(rra);
}

DEFINE_TEST(phys, rra_alloc_oom_order4) {
  struct phys_rra *rra = _phys_test_create_rra();
  TEST_ASSERT(rra);

  const size_t order = 4;

  void *pg;
  for (size_t i = 0; i < 1; ++i) {
    pg = phys_rra_alloc_order(rra, order);
    TEST_ASSERT(pg);
  }

  // Try to allocate past full. This should fail.
  TEST_ASSERT(!phys_rra_alloc_order(rra, order));

  // Free one element, see that we can allocate one more element.
  phys_rra_free_order(rra, pg, order);
  TEST_ASSERT(phys_rra_alloc_order(rra, order));
  TEST_ASSERT(!phys_rra_alloc_order(rra, order));

  // Ok not to clean up here, since the rra will be destroyed.
  _phys_test_destroy_rra(rra);
}

/**
 * Demonstrate that there may be gaps such that larger elements may not fit but
 * smaller ones can. The setup is somewhat implementation-dependent.
 */
DEFINE_TEST(phys, rra_alloc_oom_no_contiguous) {
  struct phys_rra *rra = _phys_test_create_rra();
  TEST_ASSERT(rra);

  void *pg1, *pg2, *pg3;
  TEST_ASSERT(phys_rra_alloc_order(rra, 1));       // 2
  TEST_ASSERT(phys_rra_alloc_order(rra, 1));       // 2
  TEST_ASSERT(phys_rra_alloc_order(rra, 1));       // 2
  TEST_ASSERT(pg1 = phys_rra_alloc_order(rra, 0)); // 1
  TEST_ASSERT(pg2 = phys_rra_alloc_order(rra, 0)); // 1
  TEST_ASSERT(phys_rra_alloc_order(rra, 1));       // 2
  TEST_ASSERT(phys_rra_alloc_order(rra, 1));       // 2
  TEST_ASSERT(phys_rra_alloc_order(rra, 1));       // 2
  TEST_ASSERT(phys_rra_alloc_order(rra, 0));       // 1
  TEST_ASSERT(pg3 = phys_rra_alloc_order(rra, 0)); // 1

  // Try to allocate order 0 or 1. Should fail, there's no space left.
  TEST_ASSERT(!phys_rra_alloc_order(rra, 0));
  TEST_ASSERT(!phys_rra_alloc_order(rra, 1));

  // Free one page. Try again.
  phys_rra_free_order(rra, pg2, 0);
  TEST_ASSERT(pg2 = phys_rra_alloc_order(rra, 0));
  phys_rra_free_order(rra, pg2, 0);
  TEST_ASSERT(!phys_rra_alloc_order(rra, 1));

  // Free another page (not contiguous). Try again.
  phys_rra_free_order(rra, pg3, 0);
  TEST_ASSERT(pg3 = phys_rra_alloc_order(rra, 0));
  phys_rra_free_order(rra, pg3, 0);
  TEST_ASSERT(!phys_rra_alloc_order(rra, 1));

  // Now leave a gap of order 1, so the second allocation should succeed.
  phys_rra_free_order(rra, pg1, 0);
  TEST_ASSERT(pg1 = phys_rra_alloc_order(rra, 0));
  phys_rra_free_order(rra, pg1, 0);
  TEST_ASSERT(phys_rra_alloc_order(rra, 1));

  // Ok not to clean up here, since the rra will be destroyed.
  _phys_test_destroy_rra(rra);
}

DEFINE_TEST(phys, struct_page_props) {
  struct phys_rra *rra = _phys_test_create_rra();
  TEST_ASSERT(rra);

  void *pg;
  TEST_ASSERT(pg = phys_rra_alloc_order(rra, 1));

  // Check that the struct page indicates that the page is usable and allocated.
  struct page *struct_pg = phys_rra_get_page(rra, pg);
  TEST_ASSERT(!struct_pg->unusable);
  TEST_ASSERT(struct_pg->present);

  // Check that the struct page indicates that the page is usable and not
  // allocated after freeing.
  phys_rra_free_order(rra, pg, 1);
  TEST_ASSERT(!struct_pg->unusable);
  TEST_ASSERT(!struct_pg->present);

  // Ok not to clean up here, since the rra will be destroyed.
  _phys_test_destroy_rra(rra);
}
