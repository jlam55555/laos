#ifndef UTIL_H
#define UTIL_H

// Read the hardware timestamp counter. We want this to be
// inline to decrease overhead. It may be better to have
// this as a pure assembly function, but inline assembly
// shouldn't be too far off.
inline uint64_t read_tsc(void) {
  uint64_t res;
  __asm__ volatile("rdtsc\n\t"          // Returns the time in EDX:EAX.
                   "shl $32, %%rdx\n\t" // Shift the upper bits left.
                   "or %%rdx, %0"       // 'Or' in the lower bits.
                   : "=a"(res)
                   :
                   : "rdx");
  return res;
}

#endif // UTIL_H
