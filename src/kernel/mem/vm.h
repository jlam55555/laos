/**
 * Architecture-agnostic virtual memory (VM) definitions. This depends on
 * architecture-specific definitions, such as VM_HM_START. Architecture-agnostic
 * files (such as the physical memory manager and the virtual memory manager)
 * should depend on this file rather than the architecture-specific ones.
 */
#ifndef MEM_VM_H
#define MEM_VM_H

#include "arch/x86_64/pt.h" // for VM_HM_START

// Convert addr to identity/HHDM-mapped address.
#define VM_TO_IDM(addr) (void *)((uint64_t)(addr) & ~VM_HM_START)
#define VM_TO_HHDM(addr) (void *)((uint64_t)(addr) | VM_HM_START)

#endif // MEM_VM_H
