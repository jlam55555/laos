#include <limine.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include "arch/x86_64/interrupt.h"
#include "common/libc.h"
#include "common/util.h"
#include "diag/shell.h"
#include "drivers/kbd.h"

static volatile struct limine_memmap_request _limine_memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0,
};

static void _done(void) {
  for (;;) {
    __asm__("hlt");
    shell_on_interrupt();
  }
}

__attribute__((interrupt)) void
_timer_irq(__attribute__((unused)) struct interrupt_frame *frame) {
  // Nothing to do for now.
  pic_send_eoi(0);
}

static struct kbd_driver *_kbd_driver;
__attribute__((interrupt)) void
_kb_irq(__attribute__((unused)) struct interrupt_frame *frame) {
  // Pass scancode to keyboard IRQ handler.
  // TODO(jlam55555): Make the IRQ function accept the interrupt frame
  //    rather than the character.
  _kbd_driver->kbd_irq(inb(0x60));
  pic_send_eoi(1);
}

/**
 * Kernel entry point.
 */
void _start(void) {
  // Check the Limine requests.
  if (!_limine_memmap_request.response) {
    printf("Error: limine memmap request failed\r\n");
    _done();
  }

  // Print out information about the current memory map.
  struct limine_memmap_response *limine_memmap_response =
      _limine_memmap_request.response;
  for (int i = 0; i < limine_memmap_response->entry_count; ++i) {
    struct limine_memmap_entry *mmap_entry =
        (*limine_memmap_response->entries) + i;
    char *type_str;
    switch (mmap_entry->type) {
    case LIMINE_MEMMAP_USABLE:
      type_str = "USABLE";
      break;
    case LIMINE_MEMMAP_RESERVED:
      type_str = "RESERVED";
      break;
    case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
      type_str = "ACPI_RECLAIMABLE";
      break;
    case LIMINE_MEMMAP_ACPI_NVS:
      type_str = "ACPI_NVS";
      break;
    case LIMINE_MEMMAP_BAD_MEMORY:
      type_str = "BAD_MEMORY";
      break;
    case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
      type_str = "BOOTLOADER_RECLAIMABLE";
      break;
    case LIMINE_MEMMAP_KERNEL_AND_MODULES:
      type_str = "KERNEL_AND_MODULES";
      break;
    case LIMINE_MEMMAP_FRAMEBUFFER:
      type_str = "FRAMEBUFFER";
      break;
    default:
      type_str = "UNKNOWN";
    }
    printf("base: %lx, len: %lx, type: %s\r\n", mmap_entry->base,
           mmap_entry->length, type_str);
  }

  // Initialize keyboard driver. Note that we want to do this
  // before enabling interrupts, due to the nature of the
  // keyboard initialization code.
  _kbd_driver = get_default_kbd_driver();

  // Set up basic timer and keyboard interrupts.
  create_interrupt_gate(&gates[32], _timer_irq);
  create_interrupt_gate(&gates[33], _kb_irq);
  init_interrupts();

  // Simple diagnostic shell.
  shell_init();

  // We're done, just wait for interrupt...
  _done();
}
