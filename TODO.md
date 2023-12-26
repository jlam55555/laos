# TODO
This list will probably begin as a very generic list and become more and more focused as I figure out what the heck I'm talking about.

### General
- [X] Use existing bootloader (Limine)
- [ ] Dump sys info
- [ ] Mouse IRQ and handler
- [ ] Timer driver
- [ ] Custom bootloader
- [X] Testing framework
- [ ] Automatic documentation ([Doxygen](https://www.doxygen.nl/)?)
- [ ] C++ support
- [ ] A license (this is probably really simple but I'm not sure how to choose one that is compatible with Limine)

# Terminal
- [X] Virtual terminal (vt) driver: multiplex (pseudo)terminal devices on a single text-mode screen
- [X] Keyboard IRQ and driver
- [X] Simple shell with builtin commands
- [ ] (Pseudo)terminal devices (like `/dev/ttyX`, `/dev/ptX`): manage single-channel serial I/O with a file-like interface
- [ ] Terminal multiplexing (like GNU `screen`): multiplex a single terminal device for multiple processes

### Processes
- [ ] Kernel thread scheduling
  - [X] Kernel threads
  - [X] (Cooperative) scheduling -- a `schedule()` function
  - [X] Preemptive scheduling (via timer interrupt)
- [ ] Userspace processes
  - [ ] Initial bootstrap from kernel mode into userspace.
    - [ ] Set up TSS
    - [ ] Set up userspace stack.
  - [ ] Syscall interface -- clone(), fork(), wait(), read(), write()
  - [ ] Scheduler waitqueues for different events: wait on file.
  - [ ] Userspace memory mapping/protection.
- [ ] Backtrace
- [ ] Locking primitives
- [ ] Utilities
  - [ ] Userspace shell
  - [ ] Write some basic multithreaded programs
  - [ ] ps and pstree

### Memory
- Descriptor tables
  - [ ] Set up custom GDT
  - [X] Set up custom IDT
- Physical memory utilities:
  - [X] struct page array
  - [X] Page allocator (round-robin)
  - [X] Slab allocator (simple kmalloc)
- Paging (virtual memory) utilities:
  - [X] Create a page table, map pages into it
  - [X] Swap out the page table
  - [X] Unmap pages from page table
  - [ ] mmap/nopage utilities for userspace
- [ ] Swapping

### Libraries/applications
- [ ] libc implementation
- [ ] Shared library loading
- [ ] Run ELF files
