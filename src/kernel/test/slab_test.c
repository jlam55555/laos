/**
 * Tests for the slab allocator (including `kmalloc()`/`kfree()`).
 *
 * These tests depend on environmental factors, but shouldn't be a problem
 * unless we're OOM.
 */

#include "mem/slab.h"

#include <stdint.h>

#include "test/mem_harness.h"
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
 * Test `slab_cache_{alloc,free}()`.
 */
DEFINE_TEST(slab, cache_alloc) {
  struct slab_cache *cache;
  TEST_ASSERT(cache = slab_fixture_create_slab_cache(8));

  void *obj;
  TEST_ASSERT(obj = slab_cache_alloc(cache));

  slab_cache_free(cache, NULL, obj);

  slab_fixture_destroy_slab_cache(cache);
}

/**
 * Test allocs out of order.
 */
DEFINE_TEST(slab, cache_alloc_multiple) {
  struct slab_cache *cache;
  TEST_ASSERT(cache = slab_fixture_create_slab_cache(8));

  void *obj1, *obj2, *obj3, *obj4, *obj5, *obj6, *obj7, *obj8;
  TEST_ASSERT(obj1 = slab_cache_alloc(cache));
  TEST_ASSERT(obj2 = slab_cache_alloc(cache));
  TEST_ASSERT(obj3 = slab_cache_alloc(cache));

  slab_cache_free(cache, NULL, obj2);
  slab_cache_free(cache, NULL, obj3);

  TEST_ASSERT(obj4 = slab_cache_alloc(cache));
  TEST_ASSERT(obj5 = slab_cache_alloc(cache));
  TEST_ASSERT(obj6 = slab_cache_alloc(cache));

  slab_cache_free(cache, NULL, obj1);
  slab_cache_free(cache, NULL, obj6);
  slab_cache_free(cache, NULL, obj4);

  TEST_ASSERT(obj7 = slab_cache_alloc(cache));
  TEST_ASSERT(obj8 = slab_cache_alloc(cache));

  slab_cache_free(cache, NULL, obj7);
  slab_cache_free(cache, NULL, obj5);
  slab_cache_free(cache, NULL, obj8);

  slab_fixture_destroy_slab_cache(cache);
}

/**
 * Test that adjacent allocations aren't overlapping.
 */
DEFINE_TEST(slab, cache_alloc_noverlap) {
  struct slab_cache *cache;
  TEST_ASSERT(cache = slab_fixture_create_slab_cache(8));

  // This should fill up a complete slab. 256*16=4096. It's not _too_ important
  // if this actually overflows onto another slab.
  void *objs[16];
  for (unsigned i = 0; i < 16; ++i) {
    TEST_ASSERT(objs[i] = slab_cache_alloc(cache));
  }

  for (unsigned i = 0; i < 16; ++i) {
    TEST_ASSERT_NOVERLAP2(objs[i], 256, objs[(i + 1) % 16], 256);
  }

  // Don't need to explicitly free here, destroying the slabs should cleanup.
  slab_fixture_destroy_slab_cache(cache);
}

/**
 * Make sure slab-allocated memory persists between allocs/frees. I.e., this
 * property ensures that the slab allocator can be used for caching initialized
 * objects.
 *
 * This depends on the slab allocator always allocating the last freed object,
 * which is tested in test "slab.kmalloc_last_freed_realloc".
 */
DEFINE_TEST(slab, remains_initialized_after_free_alloc_cycle) {
  struct slab_cache *cache;
  TEST_ASSERT(cache = slab_fixture_create_slab_cache(4));

  // This may not be true if we change SLAB_MIN_ORDER, so beware.
  TEST_ASSERT(cache->order == 4);

  // Need an easy 16-byte type to fill the whole object.
  struct sixteen_bytes {
    uint64_t a, b;
  } * obj1, *obj2;
  _Static_assert(sizeof(*obj1) == 16);

  TEST_ASSERT(obj1 = slab_cache_alloc(cache));

  // Fill obj1 with all 1's.
  obj1->a = obj1->b = ~0ULL;

  slab_cache_free(cache, NULL, obj1);
  TEST_ASSERT(obj2 = slab_cache_alloc(cache));
  TEST_ASSERT(obj1 == obj2);

  // Check that obj1 is still the same.
  TEST_ASSERT(obj1->a == ~0ULL && obj1->b == ~0ULL);

  // Now fill obj1 with all 0's (in case it was already filled with all 1's
  // before this test began, and the above didn't actually change the value of
  // obj1).
  obj1->a = obj1->b = 0ULL;

  slab_cache_free(cache, NULL, obj1);
  TEST_ASSERT(obj2 = slab_cache_alloc(cache));
  TEST_ASSERT(obj1 == obj2);

  // Test that obj1 is still the same.
  TEST_ASSERT(obj1->a == 0ULL && obj1->b == 0ULL);

  slab_cache_free(cache, NULL, obj1);

  slab_fixture_destroy_slab_cache(cache);
}

/**
 * Test running out of memory in the physical memory manager.
 */
DEFINE_TEST(slab, oom) {
  struct slab_cache *cache;
  TEST_ASSERT(cache = slab_fixture_create_slab_cache(8));

  // The test fixture is a order-8 (large order) slab on a 16-page physical page
  // allocator. We should be able to fit 256 entries before overflow. Note that
  // the slab descriptor for a large-order slab is allocated using `kmalloc()`
  // and doesn't factor in here.
  TEST_ASSERT(cache->elements == 16);
  TEST_ASSERT(cache->allocator->total_pg - cache->allocator->unusable_pg -
                  cache->allocator->allocated_pg ==
              16);

  void *last_alloc_obj;
  for (size_t i = 0; i < 256; ++i) {
    TEST_ASSERT(last_alloc_obj = slab_cache_alloc(cache));
  }

  // Should be full now, shouldn't be able to alloc again.
  TEST_ASSERT(!slab_cache_alloc(cache));

  // Free one element, should be able to alloc exactly once.
  slab_cache_free(cache, NULL, last_alloc_obj);
  TEST_ASSERT(slab_cache_alloc(cache));
  TEST_ASSERT(!slab_cache_alloc(cache));

  slab_fixture_destroy_slab_cache(cache);
}

/**
 * Check that no pages leak after creation, allocates/frees, and destruction.
 *
 * This duplicates some of the logic in
 * `slab_fixture_{create,destroy}_slab_cache()`.
 */
DEFINE_TEST(slab, lifecycle_leakproof) {
  struct phys_rra *rra, rra_orig;
  assert(rra = phys_fixture_create_rra());

  // Copy the rra's statistics.
  rra_orig = *rra;

  struct slab_cache *slab_cache = kmalloc(sizeof(struct slab_cache));
  slab_cache_init(slab_cache, rra, 8);

  slab_cache_destroy(slab_cache);

  TEST_ASSERT(rra_orig.total_pg == rra->total_pg);
  TEST_ASSERT(rra_orig.allocated_pg == rra->allocated_pg);
  TEST_ASSERT(rra_orig.unusable_pg == rra->unusable_pg);

  phys_fixture_destroy_rra(slab_cache->allocator);
  kfree(slab_cache);
}

/**
 * Test caches with different orders (and simultaneously).
 */
DEFINE_TEST(slab, small_order_caches) {
  struct slab_cache *cache;
  TEST_ASSERT(cache = slab_fixture_create_slab_cache(5));

  void *alloced[10];
  for (unsigned i = 0; i < 10; ++i) {
    TEST_ASSERT(alloced[i] = slab_cache_alloc(cache));
  }

  for (unsigned i = 0; i < 10; ++i) {
    slab_cache_free(cache, NULL, alloced[i]);
  }

  slab_fixture_destroy_slab_cache(cache);
}

DEFINE_TEST(slab, large_order_caches) {
  struct slab_cache *cache;
  TEST_ASSERT(cache = slab_fixture_create_slab_cache(15));

  void *alloced[2];
  for (unsigned i = 0; i < 2; ++i) {
    TEST_ASSERT(alloced[i] = slab_cache_alloc(cache));
  }

  for (unsigned i = 0; i < 2; ++i) {
    slab_cache_free(cache, NULL, alloced[i]);
  }

  slab_fixture_destroy_slab_cache(cache);
}

DEFINE_TEST(slab, multiple_caches_different_orders) {
  struct slab_cache *cache1;
  TEST_ASSERT(cache1 = slab_fixture_create_slab_cache(14));
  struct slab_cache *cache2 = kmalloc(sizeof(struct slab_cache));
  slab_cache_init(cache2, cache1->allocator, 5);
  TEST_ASSERT(cache2);

  void *objs[8];

  // Random sequence of allocations with order 14 (4KiB) and order 5 (32B)
  // caches.
  TEST_ASSERT(objs[0] = slab_cache_alloc(cache1));
  TEST_ASSERT(objs[1] = slab_cache_alloc(cache2));
  TEST_ASSERT(objs[2] = slab_cache_alloc(cache1));
  TEST_ASSERT(objs[3] = slab_cache_alloc(cache2));
  TEST_ASSERT(objs[4] = slab_cache_alloc(cache2));
  TEST_ASSERT(objs[5] = slab_cache_alloc(cache1));
  TEST_ASSERT(!(objs[6] = slab_cache_alloc(cache1)));
  TEST_ASSERT(objs[6] = slab_cache_alloc(cache2));
  TEST_ASSERT(objs[7] = slab_cache_alloc(cache2));

  slab_cache_free(cache1, NULL, objs[0]);
  slab_cache_free(cache1, NULL, objs[2]);
  slab_cache_free(cache1, NULL, objs[5]);

  slab_cache_free(cache2, NULL, objs[1]);
  slab_cache_free(cache2, NULL, objs[3]);
  slab_cache_free(cache2, NULL, objs[4]);
  slab_cache_free(cache2, NULL, objs[6]);
  slab_cache_free(cache2, NULL, objs[7]);

  slab_cache_destroy(cache2);
  kfree(cache2);
  slab_fixture_destroy_slab_cache(cache1);
}

/**
 * Try many allocations/deallocations.
 */
DEFINE_TEST(slab, stress_test) {
  struct slab_cache *cache;
  TEST_ASSERT(cache = slab_fixture_create_slab_cache(4));

  // Roughly 8KiB (2 pages).
  void **alloced = kmalloc(1000 * sizeof(void *));

  for (unsigned i = 0; i < 1000; ++i) {
    TEST_ASSERT(alloced[i] = slab_cache_alloc(cache));
  }

  for (unsigned i = 0; i < 1000; ++i) {
    slab_cache_free(cache, NULL, alloced[i]);
  }

  for (unsigned i = 0; i < 1000; ++i) {
    TEST_ASSERT(alloced[i] = slab_cache_alloc(cache));
  }

  for (unsigned i = 0; i < 1000; ++i) {
    slab_cache_free(cache, NULL, alloced[i]);
  }

  kfree(alloced);

  slab_fixture_destroy_slab_cache(cache);
}
