#include "diag/mm.h"

#include "arch/x86_64/registers.h"
#include "common/libc.h"

static int var_data = 1;
static int var_bss;

/**
 * Check if stack grows downwards using some function calls and the stack.
 * This should return true on an x86_64 (or any other mainstream) system.
 *
 * We need to turn off some optimizations:
 * - Inlining disabled using noinline.
 * - Sibling-call optimization (https://stackoverflow.com/a/54939907/)
 * - disabled using
 *   no-optimize-sibling-calls (https://stackoverflow.com/a/69328787/).
 */
__attribute__((noinline)) bool _stk_grows_downwards_helper(int *ip1) {
  int i2;
  return (long int)&i2 < (long int)ip1;
}
__attribute__((noinline, __optimize__("no-optimize-sibling-calls"))) bool
_stk_grows_downwards(void) {
  int i1;
  return _stk_grows_downwards_helper(&i1);
}

/**
 * Used to determine the approximate stack size allocated by Limine.
 * If we call this, this should recurse until we hit a NX page
 * and memory protections kill the kernel. We can subtract this from
 * the top of the stack to get a rough estimate of the stack size.
 * We expect to get some number close to and greater than 64KB; this
 * was confirmed by experiment.
 *
 * This is used to determine the bottom of the stack, since it seems
 * that Limine doesn't allocate this at a common, well-defined address.
 * Rather it uses its generic high memory allocator to allocate the stack.
 * Refer to line from Limine in common/protos/limine.c:
 *     void *stack = ext_mem_alloc(stack_size) + stack_size;
 */
__attribute__((unused, noinline,
               __optimize__("no-optimize-sibling-calls"))) void
_check_stk_size(void) {
  int x;
  printf("stksz:  0x%lx\r\n", &x);
  _check_stk_size();
}

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
 * Page-map level X table (levels 2-4).
 *
 * Used for the page directory table (level 2), the page
 * directory pointer table (level 3), and the PML4 (level 4)
 * tables. Not used for the page table (level 1).
 */
struct pmlx_entry {
  uint8_t p : 1;
  uint8_t rw : 1;
  uint8_t us : 1;
  uint8_t pwt : 1;
  uint8_t pcd : 1;
  uint8_t a : 1;
  uint8_t avl : 1;
  uint8_t ps : 1;
  uint8_t avl2 : 4;
  // Assuming a 52-bit physical address space.
  uint64_t addr : 40;
  uint16_t avl3 : 11;
  uint8_t xd : 1;
} __attribute__((packed));

/**
 * Canonicalize virtual address. For a 48-bit address space
 * (4-level paging) in x86_64, this means sign-extending bit 47
 * through to bit 63.
 */
void *va_canonicalize(void *addr) {
  if ((uint64_t)addr & 0x0000800000000000) {
    return (void *)((uint64_t)addr | 0xffff000000000000);
  }
  return addr;
}

void _print_pt(struct pmlx_entry *pmlx, int level, void *prev_va) {
  for (uint64_t i = 0; i < (4096 / sizeof *pmlx); ++i) {
    if (!pmlx[i].p) {
      continue;
    }

    void *va = va_canonicalize(
        (void *)((uint64_t)prev_va | ((uint64_t)i << (9 * level + 12))));
    void *pa = (void *)((uint64_t)pmlx[i].addr << 12);

    // Refers to a lower-level page table.
    if (level == 3 || (level && !pmlx[i].ps)) {
      _print_pt(pa, level - 1, va);
    }
    // Refers to a page.
    else {
      printf("PML%d: %lx -> %lx (%lx)\r\n", level, va, pa, pmlx);
    }
  }
}

void print_mm(void) {
  int var_stk;

  printf("\r\nMemory mapping\r\n");
  printf("Text:   0x%lx\r\n", print_mm);
  printf("Kernel: %ld bytes\r\n", -(long int)print_mm);
  printf("Data:   0x%lx\r\n", &var_data);
  printf("BSS:    0x%lx\r\n", &var_bss);
  printf("Stack:  0x%lx\r\n", &var_stk);
  printf("Stack grows downwards: %d\r\n", _stk_grows_downwards());

  struct cr3_register_pcide reg_cr3;
  _get_pt_addr(&reg_cr3);

  printf("PT:     0x%lx\r\n",
         (struct pmlx_entry *)((uint64_t)reg_cr3.base << 12));
  _print_pt((struct pmlx_entry *)((uint64_t)reg_cr3.base << 12), 3, NULL);

  printf("\r\n");
}
