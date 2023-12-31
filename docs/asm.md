# Assembly
GAS/AT&T assembly is used rather than the Intel/NASM syntax.

- gcc extended inline assembly tutorial: https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html
- OSDev entry: https://wiki.osdev.org/Inline_Assembly

## x86-64 notes

### push/pop
`%rsp` points to the last pushed value on the stack. `%rsp+8` points to the previous element. `%rsp-8` points to the position that will be pushed to next.
- `push %rxx` == `sub 8 %rsp; mov %rxx, %rsp`
- `pop %rxx` == `mov %rsp, %rxx; add 8 %rsp`

### call/ret
- `call fn` = `push %eip; jmp fn`
- `ret` = `pop %eip`

### fn prologue/epilogue
- prologue: `enter N` == `push %rbp; mov %rsp, %rbp; sub N %rsp`
  - Beforehand: `(%rsp)` == `%eip`
  - After the push: `(%rsp)` == (old) `%rbp`
  - After the sub: `(%rsp)` == last localvar
- epilogue: `leave` == `mov %rbp, %rsp; pop %rbp; ret`
  - Beforehand: `(%rsp)` == last localvar
  - After the mov: `(%rsp)` == (old) `%rbp`
  - After the pop: `(%rsp)` == (old) `%eip`

### argument order
- x86-64: `%rdi`, `%rsi`, `%rdx`, `%rcx`, `%r8`, `%r9`, and then pushed on stack in reverse order
- x86: pushed on stack in reverse order
  - Before `call`: `(%rsp)` == first arg, `(%rsp+8)` == second arg, etc.
  - After `call`: `(%rsp+8)` == first arg, ...
  - After `prologue`: `(%rbp+16)` == first arg, ...

### callee/caller-save registers
- x86_64:
  - callee-save: %rsp, %rbx, %rbp, %r12, %r13, %r14, %r15
  - caller-save: %rax, %rdi, %rsi, %rdx, %rcx, %r8, %r9, %r10, %r11

### operand sizing
- You need to be careful about the operand sizes; sometimes they are implicit and sometimes not. E.g., `mov` can usually infer it from operands, but operand-less instructions like `iret` may assume the wrong operand size (16-bit in this case). Also, `push` may cause a misaligned stack if the pushed object is smaller than the stack entry size (word size).
- rex64 prefix for 64-bit operand sizes. gcc seems to have a bug with putting this prefix on the long jump instruction (`ljmpq`), so we need to add this prefix ourselves.

### x86 documentation
It took me a while to understand how the documentation for the x86_64 ISA was laid out. It is described in detail in the [Intel Software Developer's Manual](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) (abbreviated Intel SDM). It's intimidating at >5000 pages long, but it's surprisingly digestible and navigating it is intuitive. This is broken up into three volumes:
1. **Basic architecture**: explains high-level concepts.
2. **Instruction set reference**: breaks down each opcode in detail. An alternative format can be found at https://www.felixcloutier.com/x86/ (which is scraped from the Intel SDM). For each opcode, we have the following information:
  - Available operand forms
  - Description
  - Operation (pseudocode)
  - Flags affected
  - Possible exceptions thrown
3. **System programming guide**: technical explanation of concepts and how to use them. I find this the most useful, as it carefully lays out memory layouts, constraints, etc. needed to successfully use certain features of the CPU. 

Within each volume, I've found it fairly easy to find the topic I'm looking for (whether it be segmentation, paging, exception handling, etc.). And the explanations are full of helpful visuals. Moving forward, I'll try to refer to the relevant section in comments for architecture-specific structs and code snippets.

### misc. facts
- The `naked` attribute in C implies the `noinline` and `noclone` attributes. This is because these functions tend to rely on the function call ABI (e.g., that the first argument lies in `%rdi`), which may not be true when inlined. See [initial proposal](https://lore.kernel.org/lkml/19464.59051.727647.820630@pilspetsen.it.uu.se/).
