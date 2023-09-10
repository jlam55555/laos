/**
 * Test harness and utilities for setting up a custom physical memory page
 * allocator or slab allocator. This depends on the main physical allocator (to
 * get pages for the backing buffer) and the main slab allocator (for
 * `kmalloc()`).
 *
 * TODO(jlam55555): Have a more official test fixture (setup/teardown)
 * framework.p
 */

#ifndef TEST_MEM_HARNESS_H
#define TEST_MEM_HARNESS_H

#include <stdbool.h>
#include <stddef.h>

/**
 * Returns a new RR allocator of size 16 pages. The allocator will only allocate
 * pages within the backing buffer.
 */
struct phys_rra *phys_fixture_create_rra(void);

/**
 * Cleans up a RRA allocated using `phys_test_create_rra()`.
 */
void phys_fixture_destroy_rra(struct phys_rra *rra);

struct slab_cache *slab_fixture_create_slab_cache(unsigned order);
void slab_fixture_destroy_slab_cache(struct slab_cache *slab_cache);

/**
 * Check if two regions overlap. This does not consider intervals (1, 2) and (2,
 * 3) to be overlapping, so use strict comparisons (<) rather than non-strict
 * ones (<=). See https://stackoverflow.com/a/3269471.
 */
inline bool _phys_test_overlaps(void *start1, size_t len1, void *start2,
                                size_t len2) {
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

#endif // TEST_MEM_HARNESS_H
