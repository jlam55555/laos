#ifndef IDT_H
#define IDT_H

struct idtr_desc {
  uint16_t sz;
  uint64_t off;
} __attribute__((packed));

struct segment_selector {
  uint8_t rpl : 2;
  uint8_t ti : 1;
  uint16_t index : 13;
} __attribute__((packed));

// Gate descriptor describes an ISR.
struct gate_desc {
  uint16_t off_1;
  struct segment_selector segment_selector;
  uint8_t ist : 3;
  uint8_t : 5;
  uint8_t gate_type : 4;
  uint8_t : 1;
  uint8_t dpl : 2;
  uint8_t p : 1;
  uint16_t off_2;
  uint32_t off_3;
  uint32_t : 32;
} __attribute__((packed));

// Read the IDT. See https://wiki.osdev.org/IDT
inline void read_idt(struct idtr_desc *idtr) {
  __asm__ volatile("sidt %0" : "=m"(*idtr));
}

inline void load_idtr(struct idtr_desc *idtr) {
  __asm__ volatile("lidt %0" : "=m"(*idtr));
}

#endif // IDT_H
