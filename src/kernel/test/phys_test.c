#include "mem/phys.h"

#include <limine.h>

#include "mem/slab.h"
#include "mem/vm.h" // VM_TO_HHDM
#include "test/mem_harness.h"
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

DEFINE_TEST(phys, rra_alloc) {
  struct phys_rra *rra = phys_fixture_create_rra();
  TEST_ASSERT(rra);

  void *pg = phys_rra_alloc_order(rra, 0);
  TEST_ASSERT(pg);
  phys_rra_free_order(rra, pg, 0);

  phys_fixture_destroy_rra(rra);
}

DEFINE_TEST(phys, rra_alloc_multiple) {
  struct phys_rra *rra = phys_fixture_create_rra();
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

  phys_fixture_destroy_rra(rra);
}

DEFINE_TEST(phys, rra_alloc_nonzero_order) {
  struct phys_rra *rra = phys_fixture_create_rra();
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

  phys_fixture_destroy_rra(rra);
}

DEFINE_TEST(phys, rra_alloc_oom_order0) {
  struct phys_rra *rra = phys_fixture_create_rra();
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
  phys_fixture_destroy_rra(rra);
}

DEFINE_TEST(phys, rra_alloc_oom_order1) {
  struct phys_rra *rra = phys_fixture_create_rra();
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
  phys_fixture_destroy_rra(rra);
}

DEFINE_TEST(phys, rra_alloc_oom_order4) {
  struct phys_rra *rra = phys_fixture_create_rra();
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
  phys_fixture_destroy_rra(rra);
}

/**
 * Demonstrate that there may be gaps such that larger elements may not fit but
 * smaller ones can. The setup is somewhat implementation-dependent.
 */
DEFINE_TEST(phys, rra_alloc_oom_no_contiguous) {
  struct phys_rra *rra = phys_fixture_create_rra();
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
  phys_fixture_destroy_rra(rra);
}

DEFINE_TEST(phys, struct_page_props) {
  struct phys_rra *rra = phys_fixture_create_rra();
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
  phys_fixture_destroy_rra(rra);
}

/**
 * Ensure that we can R/W the allocated pages.
 */
DEFINE_TEST(phys, page_rw) {
  struct phys_rra *rra = phys_fixture_create_rra();
  TEST_ASSERT(rra);

  void *pg;
  TEST_ASSERT(pg = phys_rra_alloc_order(rra, 1));

  uint8_t *arr = VM_TO_HHDM(pg);
  for (size_t i = 0; i < 4096; ++i) {
    arr[i] = i % 256;
  }

  for (size_t i = 0; i < 4096; ++i) {
    TEST_ASSERT(arr[i] == i % 256);
  }

  // Ok not to clean up here, since the rra will be destroyed.
  phys_fixture_destroy_rra(rra);
}
