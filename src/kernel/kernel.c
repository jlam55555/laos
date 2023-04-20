#include <limine.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include "arch/x86_64/registers.h"
/* #include "bios_reqs.h" */
#include "gdt.h"
#include "idt.h"
#include "libc.h"
/* #include "terminal.h" */
#include "drivers/console.h"
#include "drivers/kbd.h"
#include "drivers/term.h"
#include "util.h"

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent.
/* volatile struct limine_terminal_request terminal_request = { */
/*     .id = LIMINE_TERMINAL_REQUEST, .revision = 0}; */

/* struct limine_terminal *terminal = NULL; */

// This might not do anything right now with .limine_reqs not
// linked in.
/* static volatile void *volatile limine_requests[] */
/*     __attribute__((section(".limine_reqs"))) = {&terminal_request, NULL}; */

static volatile int i = 0;
static volatile char c = 0;
static void done(void) {
  for (;;) {
    __asm__("hlt");
    if (i) {
      /* printf("Got keyboard interrupt!\n"); */
      i = 0;
    }
  }
}

__attribute__((unused)) static void run_printf_tests(void) {
  // Run some printf tests.
  char buf[256];
  for (int i = 0; i < 256; ++i) {
    buf[i] = 'a';
  }
  buf[255] = 0;

  int ns[] = {
      INT_MAX,
      INT_MIN,
      UINT_MAX,
      0,
  };
  for (int i = 0, count = sizeof(ns) / sizeof(ns[0]); i < count; ++i) {
    int n = ns[i];
    int len = printf("Testing\n"
                     "%%d='%d'\n"
                     "%%u='%u'\n"
                     "%%o='%o'\n"
                     "%%b='%b'\n"
                     "%%x='%x'\n"
                     "%%c='%c'\n"
                     "%%s='%s'\n%s\n",
                     n, n, n, n, n, n, "hello", buf);
    printf("got len=%d\n", len);
  }

  long ms[] = {
      LONG_MAX,
      LONG_MIN,
      ULONG_MAX,
      0,
  };
  for (int i = 0, count = sizeof(ms) / sizeof(ms[0]); i < count; ++i) {
    long m = ms[i];
    int len = printf("Testing\n"
                     "%%ld='%ld'\n"
                     "%%lu='%lu'\n"
                     "%%lo='%lo'\n"
                     "%%lb='%lb'\n"
                     "%%lx='%lx'\n"
                     "%%s='%s'\n%s\n",
                     m, m, m, m, m, "hello", buf);
    printf("got len=%d\n", len);
  }

  char buf2[128];
  int len = snprintf(buf2, sizeof(buf2), buf);
  printf("got len=%d\n", len);
  len = printf("%s\n", buf2);
  printf("got len=%d\n", len);
}

__attribute__((unused)) static void print_gdtr_info(void) {
  struct gdtr_desc gdtr;
  struct segment_desc *seg_desc;
  int num_entries;
  size_t base, limit;

  read_gdt(&gdtr);
  printf("gdtr {\n\t.sz=0x%hx\n\t.off=0x%lx\n}\n", gdtr.sz, gdtr.off);

  // Read segment descriptors.
  num_entries = ((size_t)gdtr.sz + 1) / sizeof(*seg_desc);
  seg_desc = (struct segment_desc *)gdtr.off;
  for (; num_entries--; ++seg_desc) {
    // Compute limit. Multiply by page granularity if specified.
    limit = (size_t)seg_desc->limit_2 << 16 | seg_desc->limit_1;
    if (seg_desc->flags_g) {
      limit = ((limit + 1) << 12) - 1;
    }
    // Compute base.
    base = (size_t)seg_desc->base_3 << 24 | (size_t)seg_desc->base_2 << 16 |
           seg_desc->base_1;
    printf("segment {\n"
           "\t.limit=0x%lx\n"
           "\t.base=0x%lx\n"
           "\t.read_write=%d\n"
           "\t.accessed=%d\n"
           "\t.direction_conforming=%d\n"
           "\t.executable=%d\n"
           "\t.is_system=%d\n"
           "\t.cpu_privilege=%d\n"
           "\t.is_long_mode_code=%d\n"
           "\t.is_32bit_protected_mode=%d\n"
           "}\n",
           limit, base, seg_desc->access_rw, seg_desc->access_a,
           seg_desc->access_dc, seg_desc->access_e, seg_desc->access_s,
           seg_desc->access_dpl, seg_desc->flags_l, seg_desc->flags_db);
  }
}

static void create_interrupt_gate(struct gate_desc *gate_desc, void *isr) {
  // Select 64-bit code segment of the GDT. See
  // https://github.com/limine-bootloader/limine/blob/trunk/PROTOCOL.md#x86_64.
  gate_desc->segment_selector = (struct segment_selector){
      .rpl = 0,
      .ti = 0,
      .index = 5,
  };

  // Don't use IST.
  gate_desc->ist = 0;

  // ISR.
  gate_desc->gate_type = 0xe;

  // Run in ring 0.
  gate_desc->dpl = 0;

  // Present bit.
  gate_desc->p = 1;

  // Set offsets slices.
  gate_desc->off_1 = (size_t)isr;
  gate_desc->off_2 = ((size_t)isr >> 16) & 0xffff;
  gate_desc->off_3 = (size_t)isr >> 32;
}

// See https://wiki.osdev.org/Interrupt_Service_Routines
struct interrupt_frame {
  size_t ip;
  size_t cs;
  size_t flags;
  size_t sp;
  size_t ss;
};

// See https://wiki.osdev.org/8259_PIC
#define PIC1 0x20 /* IO base address for master PIC */
#define PIC2 0xA0 /* IO base address for slave PIC */
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1 + 1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2 + 1)
#define PIC_EOI 0x20 /* End-of-interrupt command code */
void PIC_sendEOI(unsigned char irq) {
  if (irq >= 8)
    outb(PIC_EOI, PIC2_COMMAND);

  outb(PIC_EOI, PIC1_COMMAND);
}

__attribute__((interrupt)) void
timer_irq(__attribute__((unused)) struct interrupt_frame *frame) {
  // Nothing to do for now.
  PIC_sendEOI(0);
}

static struct kbd_driver *_kbd_driver;
__attribute__((interrupt)) void
kb_irq(__attribute__((unused)) struct interrupt_frame *frame) {
  // Read scancode. (This is necessary or else the IRQ won't fire
  // again. Not sure exactly why -- may be related to:
  // https://forum.osdev.org/viewtopic.php?f=1&t=23255)
  c = inb(0x60);
  ++i;

  char buf[5];
  buf[0] = '0';
  buf[1] = 'x';
  buf[2] = "0123456789abcdef"[(c & 0xf0) >> 4];
  buf[3] = "0123456789abcdef"[c & 0xf];
  buf[4] = ' ';

  struct console_driver *console_driver = get_default_console_driver();
  console_driver->write(console_driver->dev, buf, sizeof buf);

  /* _kbd_driver->kbd_irq(c); */

  PIC_sendEOI(0);
}

static struct gate_desc gates[64] = {};
static struct idtr_desc idtr = {
    .off = (uint64_t)&gates,
    .sz = sizeof(gates) - 1,
};

__attribute__((unused)) static void print_idtr_info(void) {
  struct idtr_desc idtr;
  struct gate_desc *gate_desc;
  int num_entries;
  size_t off;

  read_idt(&idtr);
  printf("idtr {\n\t.sz=0x%hx\n\t.off=0x%lx\n}\n", idtr.sz, idtr.off);

  // Read segment descriptors.
  num_entries = ((size_t)idtr.sz + 1) / sizeof(*gate_desc);
  gate_desc = (struct gate_desc *)idtr.off;
  for (; num_entries--; ++gate_desc) {
    off = (size_t)gate_desc->off_3 << 32 | (size_t)gate_desc->off_2 << 16 |
          gate_desc->off_1;
    printf("gate {\n"
           "\t.off=0x%lx\n"
           "\t.ist=0x%lx\n"
           "\t.gate_type=0x%x\n"
           "\t.dpl=0x%x\n"
           "\t.p=0x%x\n"
           "}\n",
           off, gate_desc->ist, gate_desc->gate_type, gate_desc->dpl,
           gate_desc->p);
  }
}

__attribute__((unused)) static void create_keyboard_interrupt() {
  create_interrupt_gate(&gates[32], timer_irq);
  create_interrupt_gate(&gates[33], kb_irq);
  /* print_idtr_info(); */

  // https://forum.osdev.org/viewtopic.php?p=316295#p316295
  outb(0x11, 0x20);
  outb(0x11, 0xA0);
  outb(0x20, 0x21);
  outb(40, 0xA1);
  outb(0x04, 0x21);
  outb(0x02, 0xA1);
  outb(0x01, 0x21);
  outb(0x01, 0xA1);
  outb(0xF8, 0x21);
  outb(0xEF, 0xA1);

  load_idtr(&idtr);

  /* __asm__ volatile("int $33"); */
  __asm__ volatile("sti");
}

// note this example will always write to the top
// line of the screen
void write_string(int colour, const char *string) {
  volatile char *video = (volatile char *)0xB8000;
  while (*string != 0) {
    *video++ = *string++;
    *video++ = colour;
  }
}

// The following will be our kernel's entry point.
void _start(void) {
  /* if (terminal_request.response == NULL || */
  /*     terminal_request.response->terminal_count < 1) { */
  /*   done(); */
  /* } */
  /* terminal = terminal_request.response->terminals[0]; */

  // Initialize keyboard driver.
  _kbd_driver = get_default_kbd_driver();

  /* run_printf_tests(); */
  /* print_gdtr_info(); */
  create_keyboard_interrupt();

  /* printf("Hello, world\n"); */

  // For testing: from osdev
  /* term_set_color(0x1f); */
  /* char buf[80 * 200 + 1]; */
  /* for (int i = 0; i < 80 * 200; ++i) { */
  /*   char c = (i / 80) % 10 + '0'; */
  /*   /\* term_write(&c, 1); *\/ */
  /*   buf[i] = c; */
  /* } */
  /* buf[80 * 200] = 0; */
  /* term_writez(&buf); */

#if 1

  char buf[80 * 10];
  for (int i = 0; i < 80 * 10; ++i) {
    char c = (i / 80) % 10 + '0';
    buf[i] = c;
  }

  struct console_driver *console_driver = get_default_console_driver();
  struct console *console = console_driver->dev;
  console->cursor.color = 0x1f;
  console_driver->write(console, buf, sizeof(buf));

  struct term_driver *term_driver = get_default_term_driver();
  term_driver->master_write(term_driver->dev, "master", 6);
  term_driver->slave_write(term_driver->dev, "slave", 5);

#endif

  /* video[0] = 'a'; */
  /* video[1] = 0x1f; */
  /* video[2] = 'd'; */
  /* video[3] = 0x1f; */
  /* video[4] = 'b'; */
  /* video[5] = 0x1f; */
  /* video[6] = 'c'; */
  /* video[7] = 0x1f; */

  /* char a, b; */
  /* printf("1 -- %x %x\n", video[4], video[5]); */
  /* printf("2 -- %x %x\n", a, b); */

  /* video[4] = 0x1f; */
  /* video[5] = 'b'; */
  /* a = 0x1f; */
  /* b = 'b'; */
  /* printf("3 -- %x %x\n", video[4], video[5]); */
  /* printf("4 -- %x %x\n", a, b); */

  /* for (int i = 0; i < 25 * 80 - 1; ++i) { */
  /*   video[2 * i] = 'a' + (i % 26); */
  /*   video[2 * i + 1] = 0x02; */
  /* } */

  /* printf("hello world 2"); */

  /* write_string(0x1f, "Hello, world"); */

  /* volatile uint16_t a = detect_bios_area_hardware(); */
  /* switch (get_bios_area_video_type()) { */
  /* case VIDEO_TYPE_COLOUR: */
  /*   printf("Color\n"); */
  /*   break; */
  /* case VIDEO_TYPE_MONOCHROME: */
  /*   printf("Mono\n"); */
  /*   break; */
  /* case VIDEO_TYPE_NONE: */
  /*   printf("None\n"); */
  /*   break; */
  /* } */

  // We're done, just hang...
  done();
}
