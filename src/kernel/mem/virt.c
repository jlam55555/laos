#include "mem/virt.h"

#include <assert.h>

#include "arch/x86_64/pt.h" // VM_HM_START
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

/**
 * Helper function to map a region to a single 4KiB/2MiB page.
 *
 * `is_hugepage == true` <=> 2MiB page.
 *
 * Assumes the page isn't already mapped since there's no reason we should
 * map a virtual page twice -- this would mean there's an error in our VMM.
 *
 * TODO(jlam55555): Free memory, perhaps using refcounts.
 */
void _virt_map_page(struct pmlx_entry *pml4, void *phys_addr, void *virt_addr,
                    bool is_hugepage) {
  struct pmlx_entry *pml4e, *pml3, *pml3e, *pml2, *pml2e, *pml1, *pml1e, *pg;

  assert(PG_ALIGNED(phys_addr));
  assert(PG_ALIGNED(virt_addr));
  assert(va_is_canonical(virt_addr));

  // Get PML4 entry (PML3 table). Create if it doesn't exist. Bits [39,48) of
  // virtual address.
  // TODO(jlam55555): We can probably simplify this with more logic.
#define GET_PMLE(pml, pmle, offset)                                            \
  pmle = &pml[((size_t)virt_addr >> offset) & 0x1ff];
#define GET_PMLNEXT(pml, pmle, pmlnext, offset)                                \
  GET_PMLE(pml, pmle, offset);                                                 \
  if (!pmle->p) {                                                              \
    /* TODO(jlam55555): Turn this into a function. */                          \
    pmle->p = true;                                                            \
    pmle->addr =                                                               \
        ((size_t)_virt_alloc_pmlx_table() & (PM_MAX_BIT - 1)) >> PG_SZ_BITS;   \
  }                                                                            \
  pmlnext = (void *)((pmle->addr << PG_SZ_BITS) | VM_HM_START);

  GET_PMLNEXT(pml4, pml4e, pml3, 39);
  GET_PMLNEXT(pml3, pml3e, pml2, 30);
  GET_PMLNEXT(pml2, pml2e, pml1, 21);
  GET_PMLE(pml1, pml1e, 12);

#undef GET_PMLNEXT
#undef GET_PMLE

  // TODO(jlam55555): map page.
  // TODO(jlam55555): create intermediate ones on demand.
}

/**
 * Helper function to map a region to 4KiB and 2MiB pages, as necessary.
 *
 * Note that hugepages also need a 2MiB physical memory alignment (at least in
 * Linux): https://serverfault.com/a/898753.
 */
void _virt_map_region(struct pmlx_entry *pml4, void *phys_addr, void *virt_addr,
                      size_t len) {
  assert(PG_ALIGNED(len));
  for (size_t i = 0; i < len;) {
    // Hugepage
    bool is_hgpg = i + VM_HGPG_SZ <= len && VM_HGPG_ALIGNED(phys_addr + i) &&
                   VM_HGPG_ALIGNED(virt_addr + i);
    _virt_map_page(pml4, phys_addr + i, virt_addr + i, is_hgpg);
    i += is_hgpg ? VM_HGPG_SZ : PG_SZ;
  }
}

void _virt_create_hhdm(struct pmlx_entry *pml4,
                       struct limine_memmap_entry *init_mmap,
                       size_t entry_count) {
  for (size_t memmap_entry_i = 0; memmap_entry_i < entry_count;
       ++memmap_entry_i) {
    struct limine_memmap_entry *memmap_entry = init_mmap + memmap_entry_i;
    _virt_map_region(pml4, (void *)memmap_entry->base,
                     (void *)(VM_HM_START + memmap_entry->base),
                     memmap_entry->length);
  }
}

void _virt_create_kernel_map(struct pmlx_entry *pml4,
                             struct limine_memmap_entry *init_mmap,
                             size_t entry_count) {
  for (size_t memmap_entry_i = 0; memmap_entry_i < entry_count;
       ++memmap_entry_i) {
    struct limine_memmap_entry *memmap_entry = init_mmap + memmap_entry_i;
    if (memmap_entry->type == LIMINE_MEMMAP_KERNEL_AND_MODULES) {
      _virt_map_region(pml4, (void *)memmap_entry->base,
                       /*kernel_base=*/(void *)0xffffffff80000000,
                       memmap_entry->length);
      break;
    }
  }
}

/* __attribute__((noreturn)) */
void virt_mem_init(struct limine_memmap_entry *init_mmap, size_t entry_count) {
  // Initialize physical memory. Note that this also normalizes init_mmap (see
  // phys.c for more details).
  phys_mem_init(init_mmap, entry_count);

  // Create an entry page table.
  struct pmlx_entry *pml4 =
      (void *)((size_t)_virt_alloc_pmlx_table() | VM_HM_START);

  // Create a HHDM.
  _virt_create_hhdm(pml4, init_mmap, entry_count);

  // Create the kernel map.
  _virt_create_kernel_map(pml4, init_mmap, entry_count);

  // Reclaim bootloader memory (don't need init_mmap anymore).
  // TODO(jlam55555): Working here.

  // Switch to the new page table.
  // TODO(jlam55555): Working here.

  // Build new stack and jump to it.
  // TODO(jlam55555): Working here.
}

/* /\* __attribute__((noreturn)) *\/ void */
/* virt_mem_reclaim(struct limine_memmap_entry *init_mmap, size_t entry_count,
 */
/*                  void *(cb)(void)) { */
/*   // TODO(jlam55555): Allocate and return to new stack, as old stack was in
 */
/*   // reclaimed memory. */
/* } */

void *virt_mem_map(void *phys_ptr, size_t pg) { return NULL; }

void *virt_mem_map_noalloc(void *phys_ptr, size_t pg) { return NULL; }
