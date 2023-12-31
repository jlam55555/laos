# Bootloader

The Limine bootloader is used because it is a well-documented, modern bootloader that supports the x86_64 platform.

A custom bootloader may be written in the future to better understand the boot process, but this is secondary to learning about the kernel.

### Questions
- **Higher-half kernel**: Some reasons in [the OSDev wiki page about this](https://wiki.osdev.org/Higher_Half_Kernel).
- **HHDM (Higher Half Direct Map)**: This can be broken into two questions:
  - **Why a direct map?** It's an easy way to map all memory into the virtual address space, and gives us an easy translation between virtual and physical addresses for kernel data structures.
  - **Why in the higher-half?** See reasons for higher-half kernel. (These addresses would only be accessible by the kernel.)
  
  In Limine we have a HHDM (which begins at 0xfff800000000000). The kernel is also mapped at 0xffffffff80000000 (the highest 4GiB of memory).
- **Bootloader-reclaimable memory**: Not sure if this is a common term, but Limine uses it; these are chunks of memory that contain data structures set up by the bootloader, for certain services the bootloader provides (e.g., page tables, GDT, kernel stack, etc.). Once the kernel is done using these resources, it can reclaim these chunks of memory.
- **KASLR**: See https://lwn.net/Articles/569635/.

### Segmentation vocabulary
- **GDT (Global Descriptor Table)**: An array of GDT descriptors. We need to initialize this even though we use segmentation for memory protection, because x86_64 still uses segmentation.
- **LDT (Local Descriptor Table)**: Similar to the GDT, but can be used on a per-task basis. We don't use this as we don't use segmentation for memory protection.
- **IDT (Interrupt Descriptor Table)**: An array of gate descriptors (for interrupts/task gates).
- **TSS (Task State Segment)**: A data structure that contains information about the current task, used for hardware-based task switching and interrupt handling. Since we don't do HW task switching, this is only used for interrupt handilng (and only stores the kernel stack pointer).
- **Segment register**: Registers `cs`, `ds`, `es`, `fs`, `gs`, and `ss`. These are mostly unused nowadays, but there are some caveats. In particular, `cs` is still used to set the current privilege level (CPL). There's a lot of information about privilege levels in the Intel SDM in the explanations of memory segmentation and memory protection. These registers each contain a segment selector.
- **Segment selector**: A 16-bit field comprising a segment index (from the GDT/LDT) and a requested privilege level (RPL), which may override the CPL when performing privilege checks.
- **Segment descriptor**: A 64-bit (for non-system-segments) or 128-bit (for system segments) data structure used to describe one segment in the GDT/LDT.

### Resources
- [Inside the Linux boot process](https://developer.ibm.com/articles/l-linuxboot/)
- [Limine Protocol](https://github.com/limine-bootloader/limine/blob/trunk/PROTOCOL.md)
- [Limine Bare Bones tutorial](https://wiki.osdev.org/Limine_Bare_Bones)
- [Reasoning behind bootloader magic bytes](https://stackoverflow.com/a/1125062/)
