#include <limine.h> // for struct limine_memmap_request

#include "arch/x86_64/init.h" // for arch_init
#include "common/libc.h"      // for printf
#include "common/opcodes.h"   // for op_hlt
#include "common/util.h"      // for macro2str
#include "diag/shell.h"       // for shell_init
#include "diag/sys.h"         // for print_limine_mmap
#include "drivers/serial.h"   // for serial_init
#include "mem/virt.h"         // for virt_mem_init
#include "sched/sched.h"      // for sched_*

#ifdef RUNTEST
#include "test/test.h"
#endif // RUNTEST

static volatile struct limine_memmap_request _limine_memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0,
};

/**
 * Stage 2 of kernel initialization, after the memory manager has been set up.
 *
 * (This is a separate function from `_start` because the virtual memory
 * initialization includes setting up a new kernel stack.)
 */
static __attribute__((noreturn)) void _start_2(void) {
#ifdef RUNTEST
  run_tests(macro2str(RUNTEST));
#endif // RUNTEST

  // Bootstrap into main scheduler.
  sched_init_bootstrap();

  // Kernel initialization is done by this point. We can schedule threads to run
  // now. In the future we should just spawn the `init` process.
  //
  // For now we have the following simple setup (until we set up user-space
  // processes):
  // - Keep running the current "main" thread.
  // - Also spawn a "shell" thread.

  // Simple diagnostic shell.
  sched_new(&shell_init);

  // We're done, just wait for interrupt...
  for (;;) {
    printf("main thread\r\n");
    op_hlt();
  }
}

/**
 * Kernel entry point. The overall kernel initialization comprises the following
 * high-level steps:
 *
 * 1. Architecture-specific initialization.
 * 2. Memory management initialization (physical and virtual memory managers).
 * 3. Scheduler initialization.
 * 4. Bootstrap into the scheduler.
 *
 * After these steps, the scheduler is running with a default process and
 * interrupts are enabled, so the system is good to go.
 *
 * The physical and virtual memory manager setups depend on Limine's memmap
 * request. After the memory managers are set up, we're done with Limine's
 * bootloader services (on x86_64, this comprises the GDT, page table, kernel
 * stack, etc.) and we can reclaim Limine's bootloader-reclaimable memory. This
 * is all done in `virt_mem_init()`.
 *
 * If RUNTEST is set, then the specified tests are run after step 2.
 */
__attribute__((noreturn)) void _start(void) {
  arch_init();

  // Check the Limine requests.
  if (!_limine_memmap_request.response) {
    printf("Error: limine memmap request failed\r\n");
    for (;;) {
      op_hlt();
    }
    __builtin_unreachable();
  }
  struct limine_memmap_response *limine_memmap_response =
      _limine_memmap_request.response;

#ifdef DEBUG
  print_limine_mmap(limine_memmap_response);
#endif // DEBUG

#ifdef SERIAL
  serial_init();
#endif // SERIAL

  virt_mem_init(*limine_memmap_response->entries,
                limine_memmap_response->entry_count, _start_2);
}
