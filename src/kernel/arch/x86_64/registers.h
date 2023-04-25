#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

// See: https://wiki.osdev.org/CPU_Registers_x86-64

// General-purpose registers.
typedef uint64_t gp_reg_t;
struct gp_registers {
  gp_reg_t rax;
  gp_reg_t rbx;
  gp_reg_t rcx;
  gp_reg_t rdx;
  gp_reg_t rsi;
  gp_reg_t rdi;
  gp_reg_t rsp;
  gp_reg_t rbp;
  gp_reg_t r8;
  gp_reg_t r9;
  gp_reg_t r10;
  gp_reg_t r11;
  gp_reg_t r12;
  gp_reg_t r13;
  gp_reg_t r14;
  gp_reg_t r15;
};

// E.g., rax -> eax
inline uint32_t reg_ex(gp_reg_t reg_rx) { return reg_rx; }

// E.g., rax -> ax
inline uint16_t reg_x(gp_reg_t reg_rx) { return reg_rx; }

// E.g., rax -> ah
inline uint8_t reg_h(gp_reg_t reg_rx) { return reg_rx >> 8; }

// E.g., rax -> al
inline uint8_t reg_l(gp_reg_t reg_rx) { return reg_rx; }

struct rip_register {
  void *ip;
};

typedef uint16_t seg_reg_t;
struct segment_registers {
  seg_reg_t cs;
  seg_reg_t ds;
  seg_reg_t es;
  seg_reg_t ss;
  seg_reg_t fs;
  seg_reg_t gs;
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

// Note that there are two different interpretations
// of the cr3 register depending on the cr4.pcide flag.
// cr3_register_pcidne is for PCID not-enabled, and
// cr3_register_pcide is for PCID enabled.
struct cr3_register_pcidne {
  uint8_t : 3;
  uint8_t pwt : 1;
  uint8_t : 1;
  uint8_t pcd : 1;
  uint16_t : 6;
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

// ia32_efer, fs.base, gs.base, kernelgsbase, and tsc
// are msrs. For more information, see
// https://wiki.osdev.org/Model_Specific_Registers
// TODO: query for presence of msrs (rdmsr/wrmsr) using cpuid
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
} __attribute__((packed));

struct fsbase_msr {
  void *base;
};

struct gsbase_msr {
  void *base;
};

struct tsc_msr {
  uint64_t ts;
};

// TODO: debug registers, test registers, protected mode registers.

#endif
