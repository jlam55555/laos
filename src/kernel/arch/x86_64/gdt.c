#include "arch/x86_64/gdt.h"
#include "arch/x86_64/idt.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static struct gdt_segment_desc _gdt[7];
static const struct gdt_desc _gdt_desc = {
    .off = (uint64_t)_gdt,
    .sz = sizeof(_gdt) - 1,
};
static struct tss _tss;

static void _gdt_init_long_mode_entry(struct gdt_segment_desc *seg, bool code,
                                      int dpl) {
  // Limit, base ignored in long mode descriptor.
  seg->limit_1 = seg->limit_2 = 0;
  seg->base_1 = seg->base_2 = seg->base_3 = 0;
  seg->flags_g = 0;

  seg->access_a = 0;
  seg->access_rw = 1;
  seg->access_dc = 0;
  seg->access_e = code;
  seg->access_s = 1;
  seg->access_dpl = dpl;
  seg->access_p = 1;

  seg->flags_reserved = 0;
  seg->flags_l = code; // 1 iff long-mode code segment.
  seg->flags_db = 0;
}

static void _gdt_init_long_mode_tss_entry(struct gdt_segment_desc *seg) {
  struct gdt_system_segment_desc *tss_desc =
      (struct gdt_system_segment_desc *)seg;

  // Limit and base conversions.
  uint64_t base = (uint64_t)&_tss;
  uint32_t limit = sizeof _tss;
  tss_desc->base_1 = base;
  tss_desc->base_2 = base >> 16;
  tss_desc->base_3 = base >> 24;
  tss_desc->limit_1 = limit;
  tss_desc->limit_2 = limit >> 16;
  tss_desc->flags_g = 0; // Limit is in bytes, not pages.

  // Can be AVAIL or BUSY; this doesn't matter since we don't use HW task
  // switching.
  tss_desc->access_type = SSTYPE_TSS_AVAIL;
  tss_desc->access_s = 0;
  tss_desc->access_dpl = 0; // Doesn't matter; only used for HW task switching.
  tss_desc->access_p = 1;

  tss_desc->flags_reserved = 0;
  tss_desc->flags_l = 0;
  tss_desc->flags_db = 0;

  tss_desc->reserved = 0;

  // Zero the TSS. There aren't really any fields we need to initialize; the
  // only field we use in the TSS is rsp0.
  memset(&_tss, 0, sizeof _tss);
}

__attribute__((naked)) static void _gdt_init_ret() { __asm__ volatile("ret"); }
__attribute__((naked)) static void _gdt_init_jmp() {
  // A few notes about long jump syntax from stackoverflow.com/a/51832726:
  // - AT&T syntax uses `ljmp` rather than `jmp far`.
  // - This takes a m16:64 operand. This is written in a big-endian way -- the
  //   16 represents the high bits and the 64 represents the low bits.
  // - GCC as doesn't compile this correctly, even if we specify the 'q' suffix;
  //   we have to manually add the rex64 prefix.
  static const struct ljmp_operand {
    uint64_t offset;
    struct segment_selector selector;
  } __attribute__((packed)) op = {
      .offset = (uint64_t)_gdt_init_ret,
      .selector = (struct segment_selector){.index = GDT_SEGMENT_RING0_CODE},
  };
  __asm__("rex64 ljmp %0" : : "m"(op));
}

void gdt_init(void) {
  // Fill in GDT descriptors.
  memset(&_gdt[0], 0, sizeof _gdt[GDT_SEGMENT_NULL]);
  _gdt_init_long_mode_entry(&_gdt[GDT_SEGMENT_RING0_CODE], true, 0);
  _gdt_init_long_mode_entry(&_gdt[GDT_SEGMENT_RING0_DATA], false, 0);
  _gdt_init_long_mode_entry(&_gdt[GDT_SEGMENT_RING3_CODE], true, 3);
  _gdt_init_long_mode_entry(&_gdt[GDT_SEGMENT_RING3_DATA], false, 3);
  _gdt_init_long_mode_tss_entry(&_gdt[GDT_SEGMENT_TSS]);

  // Update the GDT and TSS descriptor to point at our new GDT/TSS.
  gdt_write(&_gdt_desc);
  tss_write(GDT_SEGMENT_TSS);

  // Update segment descriptors (except CS).
  struct segment_selector data_segment_selector = {
      .index = GDT_SEGMENT_RING0_DATA,
  };
  __asm__("mov %0, %%ds" : : "m"(data_segment_selector));
  __asm__("mov %0, %%es" : : "m"(data_segment_selector));
  __asm__("mov %0, %%ss" : : "m"(data_segment_selector));
  __asm__("mov %0, %%fs" : : "m"(data_segment_selector));
  __asm__("mov %0, %%gs" : : "m"(data_segment_selector));

  // Update CS register with a far jump.
  _gdt_init_jmp();
}

void tss_set_kernel_stack(void *rsp0) { _tss.rsp0 = rsp0; }
