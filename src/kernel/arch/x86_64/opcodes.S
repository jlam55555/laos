        .text
        .globl arch_outb
        // void arch_outb(uint8_t value, uint16_t port)
arch_outb:
        mov %rdi, %rax
        mov %rsi, %rdx
        outb %al, %dx
        ret

        .globl arch_outw
        // void arch_outw(uint16_t value, uint16_t port);
arch_outw:   
        mov %rdi, %rax
        mov %rsi, %rdx
        outw %ax, %dx
        ret

        .globl arch_inb
        // uint8_t arch_inb(uint16_t port);
arch_inb:
        mov %rdi, %rdx
        inb %dx, %al
        ret

        .globl arch_readtsc
        // uint64_t arch_readtsc(void);
arch_readtsc:
        rdtsc
        shlq $32, %rdx
        orq %rdx, %rax
        ret

        .globl arch_bsr
        // uint64_t arch_bsr(uint64_t n);
arch_bsr:
        bsrq %rdi, %rax
        ret
