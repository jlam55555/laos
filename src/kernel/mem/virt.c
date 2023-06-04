#include "mem/virt.h"

#include "mem/phys.h"

void virt_mem_init(struct limine_memmap_entry *init_mmap, size_t entry_count) {
  phys_mem_init(init_mmap, entry_count);

  // TOOD(jlam55555): Construct and swap to new VT.

  // TODO(jlam55555): Allocate and return to new stack, as old stack
  // was in reclaimed memory.
}

/* __attribute__((noreturn)) */ void
virt_mem_reclaim(struct limine_memmap_entry *init_mmap, size_t entry_count,
                 void *(cb)(void)) {}

void *virt_mem_map(void *phys_ptr, size_t pg) { return NULL; }

void *virt_mem_map_noalloc(void *phys_ptr, size_t pg) { return NULL; }
