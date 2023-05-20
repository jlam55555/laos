;; #include "common/process.h"

/**
 * Restore context.
 */
        .text
	.type	_trampoline_stack_ret, @function
_trampoline_stack_ret:
	mov _saved_rsp(%rip), %rsp
	pop %r15
	pop %r14
	pop %r13
	pop %r12
	pop %rbp
	pop %rbx
	ret
	.size	_trampoline_stack_ret, .-_trampoline_stack_ret

/**
 * Save context, set up new stack, and jump to function.
 */
	.globl	trampoline_stack
	.type	trampoline_stack, @function
trampoline_stack:
	push %rbx
	push %rbp
	push %r12
	push %r13
	push %r14
	push %r15
	mov %rsp, _saved_rsp(%rip)
	mov %rdi, %rsp
	mov %rdi, %rbp
	push $_trampoline_stack_ret
	jmp *%rsi
	.size	trampoline_stack, .-trampoline_stack

/**
 * Used to store the old stack pointer.
 */
	.bss
	.align 8
	.type	_saved_rsp, @object
	.size	_saved_rsp, 8
_saved_rsp:
	.zero	8

/**
 * Sample stack.
 */
	.globl	SAMPLE_STACK
	.align 32
	.type	SAMPLE_STACK, @object
	.size	SAMPLE_STACK, 4096
SAMPLE_STACK:
	.zero	4096
