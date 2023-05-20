# TODO
This list will probably begin as a very generic list and become more and more focused as I figure out what the heck I'm talking about.

### General
- [X] Bootstrap existing bootloader (Limine)
- [ ] Dump sys info
- [ ] Mouse IRQ and handler
- [ ] Timer driver
- [ ] Custom bootloader
- [ ] Testing framework
- [ ] Automatic documentation ([Doxygen](https://www.doxygen.nl/)?)
- [ ] Switch to clang for better tooling (e.g., already using clang-format, can also use clang-tidy, etc.)

# Terminal
- [ ] (Pseudo)terminal devices (like `/dev/ttyX`, `/dev/ptX`): manage single-channel serial I/O with a file-like interface
- [ ] Virtual terminal (vt) driver: multiplex (pseudo)terminal devices on a single text-mode screen
- [ ] Keyboard IRQ and driver
- [ ] Simple shell with builtin commands
- [ ] Terminal multiplexing (like GNU `screen`): multiplex a single terminal device for multiple processes

### Processes
- [ ] Userspace processes
- [ ] Scheduling
- [ ] Threads

### Memory
- [ ] Paging
- [ ] Swapping

### Libraries/applications
- [ ] libc implementation
- [ ] Shared library loading
- [ ] Run ELF files
