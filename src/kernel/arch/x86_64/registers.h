/**
 * Struct definitions for x86_64 registers. This can be roughly broken down into
 * 64-bit general-purpose registers (GPRs) and other userspace-accessible
 * registers, 16-bit segment registers, and 64-bit model-specific registers
 * (MSRs).
 *
 * - GPRs and other userspace-accessible registers are prefixed by 'r':
 *     - %r{a-d}x, %r{8-15}, %r{r,s}p, %r{s,d}i: GPRs. Some of these have
 *       special semantics dictated by the Intel microarchitecture, and some
 *       have special semantics based on the function call ABI.
 *     - %rip: Instruction pointer (a.k.a. program counter)
 *     - %rflags: Status register
 * - Segment registers: cs, ds, es, fs, gs, ss. In x86_64, the cs register is
 *   used to determine current privilege level (CPL).
 * - MSRs can be read/written using `rdmsr`/`wrmsr`. The `cpuid` instruction can
 *   be used to check the existence of a specific MSR.
 *
 * See: https://wiki.osdev.org/CPU_Registers_x86-64.
 *
 * The MSR specs can be found in Intel SDM Vol. 4. We don't use anything _too_
 * esoteric, but we assume at least the existence of x86_64 specific registers
 * such as IA32_EFER and IA32_LSTAR.
 */

#ifndef ARCH_X86_64_REGISTERS
#define ARCH_X86_64_REGISTERS

#include "arch/x86_64/gdt.h"
#include <stdint.h>

typedef uint64_t reg64_t;
struct regs {
  reg64_t rax;
  reg64_t rbx;
  reg64_t rcx;
  reg64_t rdx;
  reg64_t rsi;
  reg64_t rdi;
  reg64_t rbp;
  reg64_t r8;
  reg64_t r9;
  reg64_t r10;
  reg64_t r11;
  reg64_t r12;
  reg64_t r13;
  reg64_t r14;
  reg64_t r15;

  reg64_t rflags;
  reg64_t rsp;
  reg64_t rip;
};

/**
 * Read a snapshot of all the registers at this time.
 */
__attribute__((naked)) void reg_read(struct regs *regs);

/**
 * REG_PRINT is a helper macro that dumps all the registers at the current
 * point.
 *
 * See the implementation of `reg_read()` for some of the nuances behind this
 * implementation.
 */
#define _P(REG) printf("%s=0x%lx\r\n", #REG, _regs.REG);
extern struct regs _regs;
#define REG_PRINT                                                              \
  reg_read(&_regs);                                                            \
  _P(rax);                                                                     \
  _P(rbx);                                                                     \
  _P(rcx);                                                                     \
  _P(rdx);                                                                     \
  _P(rsi);                                                                     \
  _P(rdi);                                                                     \
  _P(rbp);                                                                     \
  _P(r8);                                                                      \
  _P(r9);                                                                      \
  _P(r10);                                                                     \
  _P(r11);                                                                     \
  _P(r12);                                                                     \
  _P(r13);                                                                     \
  _P(r14);                                                                     \
  _P(r15);                                                                     \
  _P(rflags);                                                                  \
  _P(rsp);                                                                     \
  _P(rip);
#undef P

// E.g., rax -> eax
inline uint32_t reg_ex(reg64_t reg_rx) { return reg_rx; }

// E.g., rax -> ax
inline uint16_t reg_x(reg64_t reg_rx) { return reg_rx; }

// E.g., rax -> ah
inline uint8_t reg_h(reg64_t reg_rx) { return reg_rx >> 8; }

// E.g., rax -> al
inline uint8_t reg_l(reg64_t reg_rx) { return reg_rx; }

struct rip_register {
  void *ip;
};

typedef uint16_t reg16_t;
struct segment_registers {
  reg16_t cs;
  reg16_t ds;
  reg16_t es;
  reg16_t ss;
  reg16_t fs;
  reg16_t gs;
};

struct rflags_register {
  uint8_t cf : 1;
  uint8_t : 1;
  uint8_t pf : 1;
  uint8_t : 1;
  uint8_t af : 1;
  uint8_t : 1;
  uint8_t zf : 1;
  uint8_t sf : 1;
  uint8_t tf : 1;
  uint8_t if_ : 1;
  uint8_t df : 1;
  uint8_t of : 1;
  uint8_t iopl : 2;
  uint8_t nt : 1;
  uint8_t : 1;
  uint8_t rf : 1;
  uint8_t vm : 1;
  uint8_t ac : 1;
  uint8_t vif : 1;
  uint8_t vip : 1;
  uint8_t id : 1;
  uint64_t : 42;
} __attribute__((packed));

struct cr0_register {
  uint8_t pe : 1;
  uint8_t mp : 1;
  uint8_t em : 1;
  uint8_t ts : 1;
  uint8_t et : 1;
  uint8_t ne : 1;
  uint16_t : 10;
  uint8_t wp : 1;
  uint8_t : 1;
  uint8_t am : 1;
  uint16_t : 10;
  uint8_t nw : 1;
  uint8_t cd : 1;
  uint8_t pg : 1;
  uint32_t : 32;
} __attribute__((packed));

struct cr2_register {
  void *pfla;
};

/**
 * Note that there are two different interpretations
 * of the cr3 register depending on the cr4.pcide flag.
 * cr3_register_pcidne is for PCID not-enabled, and
 * cr3_register_pcide is for PCID enabled.
 *
 * For most purposes, we do not care about the PCID
 * extension. According to the OSDev wiki
 * (https://wiki.osdev.org/CPU_Registers_x86):
 *     "Bits 0-11 of the physical base address are assumed
 *     to be 0. Bits 3 and 4 of CR3 are only used when
 *     accessing a PDE in 32-bit paging without PAE."
 */
struct cr3_register_pcidne {
  uint8_t : 3;
  uint8_t pwt : 1;
  uint8_t pcd : 1;
  uint16_t : 7;
  uint64_t base : 52;
} __attribute__((packed));

struct cr3_register_pcide {
  uint16_t pcid : 12;
  uint64_t base : 52;
} __attribute__((packed));

union cr3_register {
  struct cr3_register_pcidne pcidne;
  struct cr3_register_pcide pcide;
};

struct cr4_register {
  uint8_t vme : 1;
  uint8_t pvi : 1;
  uint8_t tsd : 1;
  uint8_t de : 1;
  uint8_t pse : 1;
  uint8_t pae : 1;
  uint8_t mce : 1;
  uint8_t pge : 1;
  uint8_t pce : 1;
  uint8_t osfxsr : 1;
  uint8_t osxmmexcpt : 1;
  uint8_t umip : 1;
  uint8_t : 1;
  uint8_t vmxe : 1;
  uint8_t smxe : 1;
  uint8_t : 1;
  uint8_t fsgsbase : 1;
  uint8_t pcide : 1;
  uint8_t osxsave : 1;
  uint8_t : 1;
  uint8_t smep : 1;
  uint8_t smap : 1;
  uint8_t pke : 1;
  uint8_t cet : 1;
  uint8_t pks : 1;
  uint64_t : 39;
} __attribute__((packed));

struct cr8_register {
  uint8_t priority : 4;
  uint64_t : 60;
} __attribute__((packed));

struct ia32_efer_msr {
  uint8_t sce : 1;
  uint8_t : 7;
  uint8_t lme : 1;
  uint8_t : 1;
  uint8_t lma : 1;
  uint8_t nxe : 1;
  uint8_t svme : 1;
  uint8_t lmsle : 1;
  uint8_t ffxsr : 1;
  uint8_t tce : 1;
  uint64_t : 48;
} __attribute__((packed, aligned(8)));

struct msr_ia32_star {
  uint32_t : 32;
  struct segment_selector enter_segment;
  struct segment_selector exit_segment;
} __attribute__((packed, aligned(8)));

struct fsbase_msr {
  void *base;
};

struct gsbase_msr {
  void *base;
};

struct tsc_msr {
  uint64_t ts;
};

void msr_read(uint32_t msr, uint64_t *val);
void msr_write(uint32_t msr, uint64_t val);

enum msr_address {
  MSR_IA32_EFER = 0xC0000080,  // Extended Feature Enables
  MSR_IA32_STAR = 0xC0000081,  // Syscall Target Address
  MSR_IA32_LSTAR = 0xC0000082, // Long-mode Syscall Target Address
  MSR_IA32_FMASK = 0xC0000084, // Syscall Flag Mask
};

/**
 * Enable the syscall extension and set up the syscall target address.
 *
 * TODO(jlam55555): Move this to proc/.
 */
void msr_enable_sce(void);

#endif // ARCH_X86_64_REGISTERS
