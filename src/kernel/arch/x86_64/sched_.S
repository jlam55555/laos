        .text
        // void arch_stack_switch(void **old_stk, void *new_stk);
        .globl arch_stack_switch
arch_stack_switch:
        // Save callee-save registers.
        push %rbp
        push %rbx
        push %r12
        push %r13
        push %r14
        push %r15

        // Save stack.
        mov %rsp, (%rdi)

        // The stack of a suspended or new task (created with
        // `sched_task_create()`) must look like the stack is at this point.

        // Restore stack.
        mov %rsi, %rsp

        // Restore callee-save registers.
        pop %r15
        pop %r14
        pop %r13
        pop %r12
        pop %rbx
        pop %rbp
        ret

        .globl arch_stack_jmp
        // __attribute__((noreturn)) void arch_stack_jmp(void *stk, void *rip);
arch_stack_jmp:
        mov %rdi, %rsp
        // Put a zero at the top of the call stack. Returning from the jumped-to
        // function will lead to a page fault.
        push $0
        jmp *%rsi
