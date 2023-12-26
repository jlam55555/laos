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
- [X] Kernel threads
- [ ] (Cooperative) scheduling
- [ ] Timer interrupt/preemptive scheduling
- [ ] Initial bootstrap from kernel mode into userspace
- [ ] Userspace threads/processes

WIP: processes TODO:
- Create convenience functions that use the default scheduler
- Modify the shell to run things in a different thread.
- Print process/thread tree
- Clone() and fork()
- Timer interrupt
- Write some basic multithreaded programs
- Next step: write some locking primitives (spinlock to start)
- Backtrace

### Memory
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
