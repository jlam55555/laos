#include <limine.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include "bios_reqs.h"
#include "libc.h"

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent.
volatile struct limine_terminal_request terminal_request = {
    .id = LIMINE_TERMINAL_REQUEST, .revision = 0};

struct limine_terminal *terminal = NULL;

static volatile void *volatile limine_requests[]
    __attribute__((section(".limine_reqs"))) = {&terminal_request, NULL};

static void done(void) {
  for (;;) {
    __asm__("hlt");
  }
}

static void run_printf_tests(void) {
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

// The following will be our kernel's entry point.
void _start(void) {
  if (terminal_request.response == NULL ||
      terminal_request.response->terminal_count < 1) {
    done();
  }
  terminal = terminal_request.response->terminals[0];

  // Do some basic printf tests.
  // TODO: turn these into unit tests.
  run_printf_tests();

  // We're done, just hang...
  done();
}
