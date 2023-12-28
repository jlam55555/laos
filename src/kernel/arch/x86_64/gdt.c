#include "arch/x86_64/gdt.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/**
 * Simple GDT for our purposes (all 64-bit segments). Same as that from
 * https://wiki.osdev.org/Getting_to_Ring_3#Entering_Ring_3.
 *
 *     gdt[0]  (8 bytes): null
 *     gdt[1]  (8 bytes): ring 0 code
 *     gdt[2]  (8 bytes): ring 0 data
 *     gdt[3]  (8 bytes): ring 3 code
 *     gdt[4]  (8 bytes): ring 3 data
 *     gdt[5] (16 bytes): TSS
 *
 * This is zero-initialized. Then `gdt_init()` fills in entries 1-5 and installs
 * the GDT.
 *
 * This overwrites Limine's default GDT, which lies in bootloader-reclaimable
 * memory and contains 16, 32, and 64-bit segments.
 */
static struct gdt_segment_desc _gdt[7] = {};
static const struct gdt_desc _gdt_desc = {
    .off = (uint64_t)_gdt,
    .sz = sizeof(_gdt) - 1,
};
static struct tss _tss;

static void _gdt_init_long_mode_entry(struct gdt_segment_desc *seg, bool code,
                                      bool dpl) {
  // Limit, base ignored in long mode descriptor.
  seg->limit_1 = seg->limit_2 = 0;
  seg->base_1 = seg->base_2 = seg->base_3 = 0;
  seg->flags_g = 0;

  seg->access_a = 0;
  seg->access_rw = 1;
  // TODO(jlam55555): Does this allow ring 3 code to be executed in ring 0?
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

__attribute__((naked)) static void _gdt_init_ret() {
  __asm__ volatile("ret");
  __builtin_unreachable();
}
__attribute__((naked)) static void _gdt_init_jmp() {
  static struct ljmp_operand {
    uint64_t offset;
    uint16_t selector;
  } __attribute__((packed)) op = {
      .offset = (uint64_t)_gdt_init_ret,
      .selector = 1 << 3,
  };
  // A few notes about long jump syntax from
  // https://stackoverflow.com/a/51832726:
  // - AT&T syntax uses `ljmp` rather than `jmp far`.
  // - This takes a m16:64 operand. This is written in a big-endian way -- the
  //   16 represents the high bits and the 64 represents the low bits.
  // - GCC as doesn't compile this correctly, even if we specify the 'q' suffix;
  //   we have to manually add the rex64 prefix.
  __asm__("rex64 ljmp %0" : : "m"(op));
  __builtin_unreachable();
}

void gdt_init(void) {
  // Fill in GDT descriptors.
  _gdt_init_long_mode_entry(&_gdt[1], true, 0);
  _gdt_init_long_mode_entry(&_gdt[2], false, 0);
  _gdt_init_long_mode_entry(&_gdt[3], true, 3);
  _gdt_init_long_mode_entry(&_gdt[4], false, 3);
  _gdt_init_long_mode_tss_entry(&_gdt[5]);

  // Update the GDT and TSS descriptor to point at our new GDT/TSS.
  gdt_write(&_gdt_desc);
  tss_write(5);

  // Update CS register with a far jump.
  /* _gdt_init_jmp(); */
}
