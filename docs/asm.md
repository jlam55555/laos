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
