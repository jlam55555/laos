#include "mem/virt.h"

#include <assert.h>

#include "arch/x86_64/pt.h" // VM_HM_START
#include "arch/x86_64/registers.h"
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
 */
void _virt_map_page(struct pmlx_entry *pml4, void *phys_addr, void *virt_addr,
                    bool is_hugepage) {
  struct pmlx_entry *pml4e, *pml3, *pml3e, *pml2, *pml2e, *pml1, *pml1e;

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
    pmle->rw = true;                                                           \
  }                                                                            \
  pmlnext = (void *)((pmle->addr << PG_SZ_BITS) | VM_HM_START);

  GET_PMLNEXT(pml4, pml4e, pml3, 39);
  GET_PMLNEXT(pml3, pml3e, pml2, 30);
  if (is_hugepage) {
    assert(VM_HGPG_ALIGNED(phys_addr));
    assert(VM_HGPG_ALIGNED(virt_addr));
    GET_PMLE(pml2, pml2e, 21);
    assert(!pml2e->p);
    pml2e->p = true;
    pml2e->ps = true;
    pml2e->addr = ((size_t)phys_addr & (PM_MAX_BIT - 1)) >> PG_SZ_BITS;
    pml2e->rw = true;
  } else {
    assert(PG_ALIGNED(phys_addr));
    assert(PG_ALIGNED(virt_addr));
    GET_PMLNEXT(pml2, pml2e, pml1, 21);
    GET_PMLE(pml1, pml1e, 12);
    assert(!pml1e->p);
    pml1e->p = true;
    pml1e->addr = ((size_t)phys_addr & (PM_MAX_BIT - 1)) >> PG_SZ_BITS;
    pml1e->rw = true;
  }
#undef GET_PMLNEXT
#undef GET_PMLE
}

// TODO(jlam55555): Write diagnostic function to check if a page is mapped. To
// use in debugger.

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
                     (void *)(VM_HM_START | memmap_entry->base),
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

// Switch to a new page table.
static inline void _virt_set_pt(struct pmlx_entry *pml4) {
  assert(PG_ALIGNED(pml4));
  __asm__ volatile("mov %0, %%cr3\n\t" : : "r"(pml4));
}

// Set up new stack and reclaim bootloader-reclaimable memory (in that order).
//
// Pure asm function because it's easier to do stack manipulation. Can probably
// just write it all using extended asm but it's easier to prototype in asm.
__attribute__((noreturn)) void
_virt_mem_init_stage2(struct limine_memmap_entry *init_mmap, size_t entry_count,
                      void *new_stack, void (*cb)(void));

void virt_mem_init(struct limine_memmap_entry *init_mmap, size_t entry_count,
                   void (*cb)(void)) {
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

  // Map in video memory.
  // TODO(jlam55555): Create a hardware memory map list to do this more
  // flexibly. For now we don't need to map in any other regions in the memory
  // holes.
  void *video_mem = (void *)0xB8000;
  _virt_map_region(pml4, video_mem, video_mem, PG_SZ);

  // Address should be in low-memory.
  pml4 = (void *)((size_t)pml4 & ~VM_HM_START);

  // Switch to the new page table.
  _virt_set_pt(pml4);

  // Allocate the new stack. Go to top of stack and put in HM.
  void *new_stack =
      (void *)(((uint64_t)phys_page_alloc() | VM_HM_START) + PG_SZ);

  _virt_mem_init_stage2(init_mmap, entry_count, new_stack, cb);
}
