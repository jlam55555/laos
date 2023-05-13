#include "diag/mm.h"

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
  return (int)&i2 < (int)ip1;
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

void print_mm(void) {
  int var_stk;

  printf("\r\nMemory mapping\r\n");
  printf("Text:   0x%lx\r\n", print_mm);
  printf("Kernel: %ld bytes\r\n", -(long int)print_mm);
  printf("Data:   0x%lx\r\n", &var_data);
  printf("BSS:    0x%lx\r\n", &var_bss);
  printf("Stack:  0x%lx\r\n", &var_stk);
  printf("Stack grows downwards: %d\r\n", _stk_grows_downwards());
  printf("\r\n");
}
