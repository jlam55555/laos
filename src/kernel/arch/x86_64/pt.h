/**
 * Page table/memory mapping.
 *
 * For simplicity, we assume the following constraints,
 * which are defaults set by the Limine bootloader and/or
 * the IA64 architecture.
 * - 4-level paging
 * - 48-bit virtual address space
 * - 52-bit physical address space
 *
 * This also provides utilities for generating canonical
 * virtual addresses from non-canonical ones, and checking
 * if a virtual address is canonical.
 */
#ifndef ARCH_X86_64_PT_H
#define ARCH_X86_64_PT_H

#include <stdbool.h>
#include <stdint.h>

#include "arch/x86_64/registers.h" // CR3 register

// Virtual address space size (bits).
#define VM_ADDR_SPACE_SZ 48

// Largest bit in the virtual address space.
#define VM_MAX_BIT (1lu << (VM_ADDR_SPACE_SZ - 1))

// Bits that should all be equal in a canonical
// virtual address.
#define VM_CANON_BITS (~(VM_MAX_BIT - 1))

// Number of paging levels. Assume 4-level paging
// for now.
#define VM_PG_LV 4

// Size of an ordinary (non-PSE) page.
#define VM_PG_SZ 4096
#define VM_PG_SZ_BITS 12

/**
 * Page-map level X table (levels 2-4).
 *
 * Used for the page directory table (level 2), the page
 * directory pointer table (level 3), and the PML4 (level 4)
 * tables. Not used for the page table (level 1).
 *
 * The avl* fields are ignored by the architecture, and the
 * reserved fields must not be used (set to zero).
 *
 * See IA32/64 Reference, Volume 3A, 4-32
 */
struct pmlx_entry {
  uint8_t p : 1;
  uint8_t rw : 1;
  uint8_t us : 1;
  uint8_t pwt : 1;
  uint8_t pcd : 1;
  uint8_t a : 1;
  uint8_t avl : 1;
  // Reserved on PML4, used to indicate 1GiB/2MiB pages
  // on PML3 and PML2, respectively.
  uint8_t ps : 1;
  uint8_t avl2 : 4;
  // Assuming a 52-bit physical address space.
  uint64_t addr : 40;
  uint16_t avl3 : 11;
  uint8_t xd : 1;
} __attribute__((packed));

/**
 * Get PML4 page table address.
 */
static inline void _get_pt_addr(struct cr3_register_pcide *reg_cr3) {
  __asm__ volatile("mov %%cr3, %%rax\n\t"
                   "mov %%rax, %0"
                   : "=m"(*reg_cr3)
                   :
                   : "rax");
}

/**
 * Canonicalize virtual address. For a 48-bit address space
 * (4-level paging) in x86_64, this means sign-extending bit 47
 * through to bit 63.
 */
static inline void *va_canonicalize(void *addr) {
  return (uint64_t)addr & VM_MAX_BIT ? (void *)((uint64_t)addr | VM_CANON_BITS)
                                     : addr;
}

/**
 * Check if a virtual address is canonical.
 */
static inline bool va_is_canonical(void *addr) {
  uint64_t bits = (uint64_t)addr & VM_CANON_BITS;
  return bits == VM_CANON_BITS || !bits;
}

#endif // ARCH_X86_64_PT_H
