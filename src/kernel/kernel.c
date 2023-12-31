#include <limine.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include "arch/x86_64/gdt.h"
#include "arch/x86_64/interrupt.h"
#include "arch/x86_64/registers.h"
#include "common/libc.h"
#include "common/util.h"
#include "diag/shell.h"
#include "diag/sys.h" // for print_limine_mmap
#include "drivers/kbd.h"
#include "drivers/serial.h"
#include "mem/virt.h"
#include "sched/sched.h"

#ifdef RUNTEST
#include "test/test.h"
#endif // RUNTEST

static volatile struct limine_memmap_request _limine_memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0,
};

static __attribute__((noreturn)) void _done(void) {
  for (;;) {
    __asm__("hlt");
    shell_on_interrupt();
  }
}

static __attribute__((noreturn)) void _run_shell(void) {
#ifdef RUNTEST
  run_tests(macro2str(RUNTEST));
#endif // RUNTEST

  // Bootstrap into main scheduler.
  sched_init_bootstrap();

  // Simple diagnostic shell.
  sched_new(&shell_init);

  // We're done, just wait for interrupt...
  for (;;) {
    printf("main thread\r\n");
    __asm__ volatile("hlt");
  }
}

/**
 * Kernel entry point.
 */
__attribute__((noreturn)) void _start(void) {
  // Check the Limine requests.
  if (!_limine_memmap_request.response) {
    printf("Error: limine memmap request failed\r\n");
    _done();
  }
  struct limine_memmap_response *limine_memmap_response =
      _limine_memmap_request.response;

#ifdef DEBUG
  print_limine_mmap(limine_memmap_response);
#endif // DEBUG

  // Initialize keyboard driver. Note that we want to do this before enabling
  // interrupts, since this is used in the keyboard interrupt.
  _kbd_driver = get_default_kbd_driver();

  // Set up GDT/IDT/TSS. GDT must be set up first because the TSS and IDT refer
  // to it.
  gdt_init();
  idt_init();

#ifdef SERIAL
  serial_init();
#endif // SERIAL

  // Enable and set up syscalls.
  msr_enable_sce();

  virt_mem_init(*limine_memmap_response->entries,
                limine_memmap_response->entry_count, _run_shell);
}
