#include "mem/virt.h"

#include <assert.h>

#include "arch/x86_64/pt.h"
#include "arch/x86_64/registers.h"
#include "common/libc.h"
#include "drivers/console.h"
#include "mem/phys.h"
#include "mem/slab.h"

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
        ((size_t)VM_TO_DIRECT(_virt_alloc_pmlx_table()) & (PM_MAX_BIT - 1)) >> \
        PG_SZ_BITS;                                                            \
    pmle->rw = true;                                                           \
  }                                                                            \
  pmlnext = VM_TO_HHDM(pmle->addr << PG_SZ_BITS);

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
                     VM_TO_HHDM(memmap_entry->base), memmap_entry->length);
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

void virt_mem_init(struct limine_memmap_entry *init_mmap, size_t entry_count,
                   void (*cb)(void)) {
  // Initialize physical memory. Note that this also normalizes init_mmap (see
  // phys.c for more details).
  phys_mem_init(init_mmap, entry_count);

  // Initialize slab allocator.
  slab_allocators_init();

  // Create an entry page table.
  struct pmlx_entry *pml4 = VM_TO_HHDM(_virt_alloc_pmlx_table());

  // Create a HHDM.
  _virt_create_hhdm(pml4, init_mmap, entry_count);

  // Create the kernel map.
  _virt_create_kernel_map(pml4, init_mmap, entry_count);

  // Map in video memory.
  // TODO(jlam55555): Create a hardware memory map list to do this more
  // flexibly. For now we don't need to map in any other regions in the memory
  // holes.
  void *video_mem = (void *)0xB8000;
  _virt_map_region(pml4, video_mem, VM_TO_HHDM(video_mem), PG_SZ);

  // Switch to the new page table, which should be a physical address.
  _virt_set_pt(VM_TO_DIRECT(pml4));

  // Video memory is now mapped in.
  struct console_driver *console_driver = get_default_console_driver();
  console_driver->enable(console_driver->dev);

  // Allocate the new stack. Go to top of stack and put in HM.
  void *new_stack = phys_page_alloc();
  assert(new_stack);
  new_stack += PG_SZ;

#ifdef DEBUG
  phys_mem_print_stats();
#endif // DEBUG

  // Bootloader-reclaimed memory includes the stack. This means that we should
  // immediately switch to a new stack, and nothing should write to the stack in
  // the interim. (Alternatively, we could call this after switching stacks, but
  // that shouldn't be necessary.)
  phys_reclaim_bootloader_mem(init_mmap, entry_count);

#ifdef DEBUG
  phys_mem_print_stats();
#endif // DEBUG

  // Switch stacks. We technically clobber %rsp, but listing it as a clobber
  // gives a warning. Similarly, we cannot use memory-mode addressing for cb, as
  // this will use %rsp-relative addressing. Just specifying a register operand
  // is probably still not totally safe (if the register operand is set from a
  // %rsp-relative address after %rsp is clobbered), but safe enough for our
  // needs.
  __asm__ volatile("mov %0, %%rsp\n\t" /* set stack  */
                   "push $0\n\t"       /* push fake %rip */
                   "jmp *%1\n\t"       /* jump to cb */
                   :
                   : "r"(new_stack), "r"(cb));

  __builtin_unreachable();
}
