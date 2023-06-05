#include "mem/virt.h"

#include <assert.h>

#include "common/libc.h"
#include "mem/phys.h"

/**
 * Allocates and returns a pointer to an empty (zeroed) PMLx table.
 */
struct pmlx_entry *_virt_alloc_pmlx_table(void) {
  struct pmlx_entry *rv;
  assert(rv = phys_page_alloc());
  memset(rv, 0, PG_SZ);
  return rv;
}

// TODO(jlam55555)
void _virt_map_page(struct pmlx_entry *pml4, void *virt_addr, void *phys_addr) {
}

void _virt_create_hhdm(struct pmlx_entry *pml4) {
  // TODO(jlam55555): Loop through physical memory, map each page to a 4KiB or
  // 2MiB page.
}

void _virt_create_kernel_map(struct pmlx_entry *pml4,
                             struct limine_memmap_entry *init_mmap,
                             size_t entry_count) {
  // TODO(jlam55555): Loop through each page of the init_mmap, map kernel and
  // bootloader-reclaimable regions.
}

void virt_mem_init(struct limine_memmap_entry *init_mmap, size_t entry_count) {
  // Initialize physical memory. Note that this also normalizes init_mmap (see
  // phys.c for more details).
  phys_mem_init(init_mmap, entry_count);

  // Create an entry page table.
  struct pmlx_entry *pml4 = _virt_alloc_pmlx_table();

  // Create a HHDM.
  _virt_create_hhdm(pml4);

  // Create the kernel map.
  _virt_create_kernel_map(pml4, init_mmap, entry_count);
}

/* __attribute__((noreturn)) */ void
virt_mem_reclaim(struct limine_memmap_entry *init_mmap, size_t entry_count,
                 void *(cb)(void)) {
  // TODO(jlam55555): Allocate and return to new stack, as old stack was in
  // reclaimed memory.
}

void *virt_mem_map(void *phys_ptr, size_t pg) { return NULL; }

void *virt_mem_map_noalloc(void *phys_ptr, size_t pg) { return NULL; }
