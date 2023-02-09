#ifndef GDT_H
#define GDT_H

struct gdtr_desc {
  uint16_t sz;
  uint64_t off;
} __attribute__((packed));

struct segment_desc {
  uint16_t limit_1;
  uint16_t base_1;
  uint8_t base_2;
  uint8_t access_a : 1;
  uint8_t access_rw : 1;
  uint8_t access_dc : 1;
  uint8_t access_e : 1;
  uint8_t access_s : 1;
  uint8_t access_dpl : 2;
  uint8_t access_p : 1;
  uint8_t limit_2 : 4;
  uint8_t flags_reserved : 1;
  uint8_t flags_l : 1;
  uint8_t flags_db : 1;
  uint8_t flags_g : 1;
  uint8_t base_3;
} __attribute__((packed));

// Read the GDT.
// See also:
// - Sample sgdt usage: https://stackoverflow.com/q/57196984/2397327
// - sgdt instruction reference: https://www.felixcloutier.com/x86/sgdt
// - GDT layout: https://wiki.osdev.org/Global_Descriptor_Table
// - Similar code in the Linux kernel:
//   arch/x86/include/asm/desc_defs.h
inline void read_gdt(struct gdtr_desc *gdtr) {
  __asm__ volatile("sgdt %0" : "=m"(*gdtr));
}

#endif // GDT_H
