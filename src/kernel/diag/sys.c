#include "diag/sys.h"

#include <limits.h>
#include <stdint.h>

#include "arch/x86_64/gdt.h"
#include "arch/x86_64/idt.h"
#include "common/libc.h"

// TODO(jlam55555): Remove this. Or move to a unit test (and probably use
// sprintf rather than printf).
void run_printf_tests(void) {
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
    int len = printf("Testing\r\n"
                     "%%d='%d'\r\n"
                     "%%u='%u'\r\n"
                     "%%o='%o'\r\n"
                     "%%b='%b'\r\n"
                     "%%x='%x'\r\n"
                     "%%c='%c'\r\n"
                     "%%s='%s'\r\n%s\r\n",
                     n, n, n, n, n, n, "hello", buf);
    printf("got len=%d\r\n", len);
  }

  long ms[] = {
      LONG_MAX,
      LONG_MIN,
      ULONG_MAX,
      0,
  };
  for (int i = 0, count = sizeof(ms) / sizeof(ms[0]); i < count; ++i) {
    long m = ms[i];
    int len = printf("Testing\r\n"
                     "%%ld='%ld'\r\n"
                     "%%lu='%lu'\r\n"
                     "%%lo='%lo'\r\n"
                     "%%lb='%lb'\r\n"
                     "%%lx='%lx'\r\n"
                     "%%s='%s'\r\n%s\r\n",
                     m, m, m, m, m, "hello", buf);
    printf("got len=%d\r\n", len);
  }

  char buf2[128];
  int len = snprintf(buf2, sizeof(buf2), buf);
  printf("got len=%d\r\n", len);
  len = printf("%s\r\n", buf2);
  printf("got len=%d\r\n", len);
}

// TODO(jlam55555): saa
void print_gdtr_info(void) {
  struct gdt_desc gdtr;
  struct gdt_segment_desc *seg_desc;
  int num_entries;
  size_t base, limit;

  read_gdt(&gdtr);
  printf("gdtr {\r\n\t.sz=0x%hx\r\n\t.off=0x%lx\r\n}\r\n", gdtr.sz, gdtr.off);

  // Read segment descriptors.
  num_entries = ((size_t)gdtr.sz + 1) / sizeof(*seg_desc);
  seg_desc = (struct gdt_segment_desc *)gdtr.off;
  for (; num_entries--; ++seg_desc) {
    // Compute limit. Multiply by page granularity if specified.
    limit = (size_t)seg_desc->limit_2 << 16 | seg_desc->limit_1;
    if (seg_desc->flags_g) {
      limit = ((limit + 1) << 12) - 1;
    }
    // Compute base.
    base = (size_t)seg_desc->base_3 << 24 | (size_t)seg_desc->base_2 << 16 |
           seg_desc->base_1;
    printf("segment {\r\n"
           "\t.limit=0x%lx\r\n"
           "\t.base=0x%lx\r\n"
           "\t.read_write=%d\r\n"
           "\t.accessed=%d\r\n"
           "\t.direction_conforming=%d\r\n"
           "\t.executable=%d\r\n"
           "\t.is_system=%d\r\n"
           "\t.cpu_privilege=%d\r\n"
           "\t.is_long_mode_code=%d\r\n"
           "\t.is_32bit_protected_mode=%d\r\n"
           "}\r\n",
           limit, base, seg_desc->access_rw, seg_desc->access_a,
           seg_desc->access_dc, seg_desc->access_e, seg_desc->access_s,
           seg_desc->access_dpl, seg_desc->flags_l, seg_desc->flags_db);
  }
}

// TODO(jlam55555): saa
void print_idtr_info(void) {
  struct idtr_desc idtr;
  struct gate_desc *gate_desc;
  int num_entries;
  size_t off;

  read_idt(&idtr);
  printf("idtr {\r\n\t.sz=0x%hx\r\n\t.off=0x%lx\r\n}\r\n", idtr.sz, idtr.off);

  // Read segment descriptors.
  num_entries = ((size_t)idtr.sz + 1) / sizeof(*gate_desc);
  gate_desc = (struct gate_desc *)idtr.off;
  for (; num_entries--; ++gate_desc) {
    off = (size_t)gate_desc->off_3 << 32 | (size_t)gate_desc->off_2 << 16 |
          gate_desc->off_1;
    printf("gate {\r\n"
           "\t.off=0x%lx\r\n"
           "\t.ist=0x%lx\r\n"
           "\t.gate_type=0x%x\r\n"
           "\t.dpl=0x%x\r\n"
           "\t.p=0x%x\r\n"
           "}\r\n",
           off, gate_desc->ist, gate_desc->gate_type, gate_desc->dpl,
           gate_desc->p);
  }
}
