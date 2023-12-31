#include "mem/virt.h"

#include <assert.h>

#include "arch/x86_64/pt.h"    // for arch_pt_init
#include "arch/x86_64/sched.h" // for arch_stack_jmp
#include "drivers/console.h"   // for get_default_console_driver
#include "mem/phys.h"          // for phys_alloc_page
#include "mem/slab.h"          // for slab_allocators_init
#include "mem/vm.h"            // for VM_TO_IDM, VM_TO_HHDM

void virt_mem_init(struct limine_memmap_entry *init_mmap, size_t entry_count,
                   void (*cb)(void)) {
  // Initialize physical memory. Note that this also normalizes init_mmap (see
  // phys.c for more details).
  phys_mem_init(init_mmap, entry_count);

  // Initialize slab allocator.
  slab_allocators_init();

  // Set up architecture-specific page table.
  arch_pt_init(init_mmap, entry_count);

  // Video memory is now mapped in.
  struct console_driver *console_driver = get_default_console_driver();
  console_driver->enable(console_driver->dev);

  // Allocate the new stack. Go to top of stack and put in HM.
  void *new_stack = phys_alloc_page();
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

  // Jump to new stack.
  arch_stack_jmp(new_stack, cb);
  __builtin_unreachable();
}
