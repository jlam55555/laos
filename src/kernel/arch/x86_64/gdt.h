/**
 * Global Descriptor Table (GDT) data structure for segment protections on the
 * x86_64 architecture.
 *
 * Luckily Limine sets this up with reasonable defaults. The definitions below
 * may be used to print out diagnostic information about the GDT.
 */
#ifndef ARCH_X86_64_GDT_H
#define ARCH_X86_64_GDT_H

#include "common/util.h" // for static_assert

struct gdt_desc {
  uint16_t sz;
  uint64_t off;
} __attribute__((packed));
static_assert(sizeof(struct gdt_desc) == 10, "sizeof gdt_desc");

/**
 * Note that reserved fields are named so that we can explicitly zero them if
 * necessary.
 */
struct gdt_segment_desc {
  uint16_t limit_1;
  uint16_t base_1;
  uint8_t base_2;
  uint8_t access_a : 1;
  uint8_t access_rw : 1;
  uint8_t access_dc : 1;
  uint8_t access_e : 1;
  uint8_t access_s : 1; // Always 1 for non-system segments.
  uint8_t access_dpl : 2;
  uint8_t access_p : 1;
  uint8_t limit_2 : 4;
  uint8_t flags_reserved : 1;
  uint8_t flags_l : 1;
  uint8_t flags_db : 1;
  uint8_t flags_g : 1;
  uint8_t base_3;
} __attribute__((packed));
static_assert(sizeof(struct gdt_segment_desc) == 8, "sizeof gdt_segment_desc");

/**
 * 64-bit mode type field, as described in Sec. 3.5. Table 3-2.
 *
 * This is also used for IDT gate entries.
 */
enum system_segment_type {
  SSTYPE_LDT = 0x2,
  SSTYPE_TSS_AVAIL = 0x9,
  SSTYPE_TSS_BUSY = 0xB,
  SSTYPE_CALL_GATE = 0xC,
  SSTYPE_INTERRUPT_GATE = 0xE,
  SSTYPE_TRAP_GATE = 0xF,
};

/**
 * 64-bit mode system segment (LDT or TSS) descriptor. This is 16 bytes rather
 * than 8 bytes like the 32-bit version.
 *
 * Described in Intel SDM, Vol. 3-A, Sec. 3.5.
 */
struct gdt_system_segment_desc {
  uint16_t limit_1;
  uint16_t base_1;
  uint8_t base_2;
  enum system_segment_type access_type : 4;
  uint8_t access_s : 1; // Always 0 for system segments.
  uint8_t access_dpl : 2;
  uint8_t access_p : 1;
  uint8_t limit_2 : 4;
  uint8_t flags_reserved : 1;
  uint8_t flags_l : 1;
  uint8_t flags_db : 1;
  uint8_t flags_g : 1;
  uint64_t base_3 : 40;
  uint32_t reserved : 32;
} __attribute__((packed));
static_assert(sizeof(struct gdt_system_segment_desc) == 16,
              "sizeof gdt_system_segment_desc");

struct tss {
  uint32_t reserved_1;
  uint64_t rsp0;
  uint64_t rsp1;
  uint64_t rsp2;
  uint64_t reserved_2;
  uint64_t ist1;
  uint64_t ist2;
  uint64_t ist3;
  uint64_t ist4;
  uint64_t ist5;
  uint64_t ist6;
  uint64_t ist7;
  uint64_t reserved_3;
  uint16_t reserved_4;
  uint16_t iopb;
} __attribute__((packed));
static_assert(sizeof(struct tss) == 104, "sizeof tss");

/**
 * Read/write the GDT. When writing the GDT, make sure to also update segment
 * registers. Even though they are cached (hidden part), they will be re-read
 * from the GDT on some instructions based on the visible part, which may not be
 * correct anymore.
 */
inline void read_gdt(struct gdt_desc *gdt_desc) {
  __asm__ volatile("sgdt %0" : "=m"(*gdt_desc));
}
inline void gdt_write(const struct gdt_desc *const gdt_desc) {
  __asm__ volatile("lgdt %0" : : "m"(*gdt_desc));
}

/**
 * Write the TSS.
 */
inline void tss_write(uint16_t tss_segment_index) {
  // Set TI, DPL to 0.
  uint16_t tss_segment_selector = tss_segment_index << 3;
  __asm__ volatile("ltr %0" : : "m"(tss_segment_selector) : "ax");
}

/**
 * Set up a new GDT.
 *
 * TODO(jlam55555): move documentation here.
 */
void gdt_init(void);

#endif // ARCH_X86_64_GDT_H
