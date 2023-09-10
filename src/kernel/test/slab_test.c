/**
 * Tests for the slab allocator (including `kmalloc()`/`kfree()`).
 *
 * These tests depend on environmental factors, but shouldn't be a problem
 * unless we're OOM.
 *
 * TODO(jlam55555): Define lifecycle tests for test initialization and cleanup.
 * Cleanup is more important, because initialization can happen anywhere in the
 * test.
 */

#include "mem/slab.h"

#include "test/phys_fixture.h"
#include "test/test.h"

/**
 * Like PG_ALIGNED, but for an arbitrary power-of-two sz. This macro also checks
 * that sz is a power-of-two, but perhaps that's overkill.
 */
#define ALIGNED(ptr, sz) (!(sz & (sz - 1)) && !((uint64_t)ptr & (sz - 1)))

DEFINE_TEST(slab, kmalloc_aligned) {
  void *alloc1 = kmalloc(16);
  TEST_ASSERT(alloc1);
  TEST_ASSERT(ALIGNED(alloc1, 16));
  kfree(alloc1);

  void *alloc2 = kmalloc(32);
  TEST_ASSERT(alloc2);
  TEST_ASSERT(ALIGNED(alloc2, 32));
  kfree(alloc2);
}

DEFINE_TEST(slab, kmalloc_unaligned) {
  void *alloc1 = kmalloc(5);
  TEST_ASSERT(alloc1);
  TEST_ASSERT(ALIGNED(alloc1, 8));
  kfree(alloc1);

  void *alloc2 = kmalloc(17);
  TEST_ASSERT(alloc2);
  TEST_ASSERT(ALIGNED(alloc2, 32));
  kfree(alloc2);

  void *alloc3 = kmalloc(511);
  TEST_ASSERT(alloc3);
  TEST_ASSERT(ALIGNED(alloc3, 512));
  kfree(alloc3);

  void *alloc4 = kmalloc(1203);
  TEST_ASSERT(alloc4);
  TEST_ASSERT(ALIGNED(alloc4, 2048));
  kfree(alloc4);
}

#undef ALIGNED

DEFINE_TEST(slab, kmalloc_extreme_orders) {
  // Can alloc largest alloc order.
  void *alloc1 = kmalloc(1u << SLAB_MAX_ORDER);
  TEST_ASSERT(alloc1);
  kfree(alloc1);

  // Cannot alloc larger than the largest alloc orde.
  void *alloc2 = kmalloc(1u << (SLAB_MAX_ORDER + 1));
  TEST_ASSERT(!alloc2);

  // Can alloc smallest alloc order.
  void *alloc3 = kmalloc(1u << SLAB_MIN_ORDER);
  TEST_ASSERT(alloc3);
  kfree(alloc3);

  // Can alloc smaller than the smallest alloc order (will get rounded up).
  void *alloc4 = kmalloc(1u << (SLAB_MIN_ORDER - 1));
  TEST_ASSERT(alloc4);
  kfree(alloc4);
}

DEFINE_TEST(slab, kmalloc_not_same_address) {
  void *alloc1 = kmalloc(16);
  TEST_ASSERT(alloc1);
  void *alloc2 = kmalloc(16);
  TEST_ASSERT(alloc2);

  TEST_ASSERT(alloc1 != alloc2);

  kfree(alloc1);
  kfree(alloc2);
}

/**
 * Test that the last freed object will be the next object allocated. This is a
 * guarantee of the slab allocator.
 */
DEFINE_TEST(slab, kmalloc_last_freed_realloc) {
  void *alloc1 = kmalloc(16);
  TEST_ASSERT(alloc1);
  kfree(alloc1);

  void *alloc2 = kmalloc(16);
  TEST_ASSERT(alloc2);
  TEST_ASSERT(alloc1 == alloc2);

  kfree(alloc2);
}

/**
 * Test `slab_{alloc,free}()`.
 */
DEFINE_TEST(slab, alloc) {
  struct slab_cache *cache;
  TEST_ASSERT(cache = slab_fixture_create_slab_cache(8));

  void *v;
  TEST_ASSERT(v = slab_cache_alloc(cache));

  slab_cache_free(cache, NULL, v);

  slab_fixture_destroy_slab_cache(cache);
}

/**
 * Test allocs out of order.
 */
DEFINE_TEST(slab, alloc_multiple) { TEST_ASSERT(false); }

/**
 * Make sure slab-allocated memory is R/W-able.
 */
DEFINE_TEST(slab, alloc_rwable) { TEST_ASSERT(false); }

/**
 * Make sure slab-allocated memory persists between allocs/frees. I.e., this
 * property ensures that the slab allocator can be used for caching initialized
 * objects.
 *
 * This depends on the slab allocator always allocating the last freed object,
 * which is tested in test "slab.kmalloc_last_freed_realloc".
 */
DEFINE_TEST(slab, remains_initialized_after_free_alloc_cycle) {
  TEST_ASSERT(false);
}

/**
 * Test `slab_cache_{alloc,free}()`.
 */
DEFINE_TEST(slab, cache_alloc) { TEST_ASSERT(false); }

/**
 * Test running out of memory in the physical memory manager.
 */
DEFINE_TEST(slab, oom) { TEST_ASSERT(false); }

/**
 * Check that no pages leak after creation, allocates/frees, and destruction.
 */
DEFINE_TEST(slab, lifecycle_leakproof) { TEST_ASSERT(false); }

/**
 * Test caches with different orders (and simultaneously).
 */
DEFINE_TEST(slab, small_order_caches) { TEST_ASSERT(false); }
DEFINE_TEST(slab, large_order_caches) { TEST_ASSERT(false); }
DEFINE_TEST(slab, multiple_caches_different_orders) { TEST_ASSERT(false); }
