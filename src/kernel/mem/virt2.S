        .global _virt_mem_init_stage2

        .text

_virt_mem_init_stage2:
        # Save cb in callee-save register.
        mov %rcx, %rbx
        
        # Set new stack.
        mov %rdx, %rsp

        # Cleanup bootloader_reclaimable memory.
        call phys_reclaim_bootloader_mem

        # Push fake %rip onto the stack.
        push $0
        
        # Jump into the callback function.
        jmp %rbx
