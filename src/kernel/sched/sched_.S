        .text
        // void _sched_task_switch_stack(void **old_stk, void *new_stk);
        .globl _sched_task_switch_stack
_sched_task_switch_stack:
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
