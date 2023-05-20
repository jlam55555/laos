# laos -- LAm Operating System

A personal study on operating systems, with help from the OSDev Wiki. A long-term weekend side project.

### Quick start

##### Build instructions

Make sure to have [QEMU][qemu] and [GNU `make`][make] installed.

```bash
# Fetch Limine (bootloader) dependency.
git submodule update --init

# Build Limine (this build is separate from the kernel build)
make limine

# Build and run in QEMU (run_iso target also available)
sudo make run_hdd

# Cleanup
sudo make limine-clean clean
```

##### Debugging

See the [OSDev entry on kernel debugging](https://wiki.osdev.org/Kernel_Debugging#Use_GDB_with_QEMU).
```bash
# Make sure to purge old builds.
sudo make clean

# Build with debugging symbols.
sudo make kernel_debug out/image.hdd

# Run with debugging.
sudo qemu-system-x86_64 -s -S out/image.hdd

# Connect GDB.
# (Perhaps run this in a separate window.)
gdb -ex "target remote localhost:1234" -ex "file out/kernel/myos.elf"
```

### Features
- [X] Limine bootloader (forked to jlam55555/limine to enable VGA text mode 3 by default)
- [X] PS/2 Keyboard driver: read scancodes, translate to keycodes/ASCII and pass to input subsystem
- [X] TTY subsystem: basic line discipline, buffering core utilities, tty driver
- [X] Console driver: write text to screen, with scrollback; interpret special characters like `^H`, `^M`, `^J`
- [X] Simple shell: run simple hardcoded commands
- [ ] Synchronization primitives: mask interrupts/save IRQ state, spinlocks (for multicore)
- [ ] Memory management: page fault handler, page table representation, page allocator, library functions for heap allocation
- [ ] Processes: TODO

### Learning goals

I.e., interest points.

- The bootloader
- Memory and cache management (e.g., fences, cache coherence policies, etc.)
- Scheduling on a multiprocessor system, especially focusing on latencies and power requirements on a RTS-like system
- Operating characteristics of popular ARM processors (e.g., ARM Cortex R52); tracing and observing real latencies at high precision. (However, an initial port to x86-64 may be simpler)
- Interrupt handling and latencies
- Integrating Rust as a systems language

### Non-goals

- Creating a userspace application environment
- POSIX compliance (or any other compliance)
- UI system (except maybe an X clone)
- Becoming the next Linux (or Windows, or Mac OS)

### Other projects with this name

- [Latency Aware Operating System][laos-2]

[laos-2]: https://www4.cs.fau.de/Research/LAOS/
[qemu]: https://wiki.archlinux.org/title/QEMU
[make]: https://www.gnu.org/software/make/
