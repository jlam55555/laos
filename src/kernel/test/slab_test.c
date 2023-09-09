#include "mem/slab.h"

#include "test/test.h"

/**
 * Like PG_ALIGNED, but for an arbitrary power-of-two sz.
 *
 * This macro also checks that sz is a power-of-two, but that's not strictly
 * necessary.
 */
#define ALIGNED(ptr, sz) (!(sz & (sz - 1)) && !((uint64_t)ptr & (sz - 1)))

/** This test depends on environmental factors. If you run low on space, this
 * will fail. This should be made more hermetic by initializing separate slab
 * allocators outside of the normal MM system. On a regular machine with plenty
 * of memory, this should succeed just fine. If it starts failing, we have a
 * problem (and also some unfreed memory)!
 *
 * TODO(jlam55555): Define lifecycle tests for test initialization and cleanup.
 * Cleanup is more important, because initialization can happen anywhere in the
 * test.
 */
DEFINE_TEST(slab, can_alloc_aligned) {
  void *alloc1 = kmalloc(16);
  TEST_ASSERT(alloc1);
  TEST_ASSERT(ALIGNED(alloc1, 16));
  kfree(alloc1);

  void *alloc2 = kmalloc(32);
  TEST_ASSERT(alloc2);
  TEST_ASSERT(ALIGNED(alloc1, 32));
  kfree(alloc2);
}

DEFINE_TEST(slab, can_alloc_unaligned) {
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

DEFINE_TEST(slab, min_max_alloc_size) {
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

DEFINE_TEST(slab, alloc_not_same) {
  void *alloc1 = kmalloc(16);
  TEST_ASSERT(alloc1);
  void *alloc2 = kmalloc(16);
  TEST_ASSERT(alloc2);

  TEST_ASSERT(alloc1 != alloc2);

  kfree(alloc1);
  kfree(alloc2);
}

/**
 * Test that the last freed object will be the next object allocated.
 *
 * Our slab allocator doesn't have to guarantee this, but it's a nice property
 * to have.
 */
DEFINE_TEST(slab, alloc_last_freed) {
  void *alloc1 = kmalloc(16);
  TEST_ASSERT(alloc1);
  kfree(alloc1);

  void *alloc2 = kmalloc(16);
  TEST_ASSERT(alloc2);
  TEST_ASSERT(alloc1 == alloc2);

  kfree(alloc2);
}

// TODO(jlam55555): Tests to ensure that memory is R/W-able, run out of memory,
// etc.
// TODO(jlam55555): Internal unit tests to make sure that slab bookkeeping is
// correct.
// TODO(jlam55555): Add unit tests for _slab_allocator_init,
// _slab_allocator_get_cache, _slab_cache_alloc_slab,
// _slab_cache_find_nonfull_slab, _slab_move_to_ll, _slab_cache_alloc,
// _slab_free, _slab_cache_free
