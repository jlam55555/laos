/**
 * Set up a custom physical memory allocator. This depends on the main physical
 * allocator (to get pages for the backing buffer) and the main slab allocator
 * (for `kmalloc()`).
 *
 * TODO(jlam55555): Have a more official test fixture (setup/teardown)
 * framework.
 *
 * TODO(jlam55555): Rename this file since we also test slab allocator stuff
 * here.
 */

#ifndef TEST_PHYS_FIXTURE_H
#define TEST_PHYS_FIXTURE_H

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

#endif // TEST_PHYS_FIXTURE_H
