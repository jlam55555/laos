#include "diag/sys.h"

#include <limits.h>
#include <stdint.h>

#include "arch/x86_64/gdt.h"
#include "arch/x86_64/idt.h"
#include "common/libc.h"

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

void print_gdtr_info(void) {
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

void print_idtr_info(void) {
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
