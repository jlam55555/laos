# laos -- LAm Operating System

A personal study on operating systems, with help from the OSDev Wiki. A long-term weekend side project.

### Quick start

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

##### More advanced usage
```
# Run with extra logging and debug flags.
sudo make DEBUG=1 run_hdd

# Run all tests.
sudo make RUNTEST=all run_hdd

# Run tests that match the specified pattern.
sudo make RUNTEST=^slab.,^phys. run_hdd

# Run specified tests with debug flags.
sudo make RUNTEST

# Run interactively with gdb and debug flags/logging. Run these in separate
# terminal emulators, and make sure the variants match exactly.
sudo make RUNTEST=^list. DEBUG=i run_hdd
make RUNTEST=^list. DEBUG=i run_gdb
```

Each `RUNTEST` value creates a different build variant to avoid inconsistent builds. Specifying `DEBUG` also creates a different build variant. Output binaries, intermediate files, and test results will be written to `out<build_variant>/`.

TODO(jlam55555): Note that headers are not correctly considered as dependencies, and the debug mode still has the `-O2` flag because there are problems building with `-O0`.

### Features
- [X] Limine bootloader (forked to jlam55555/limine to enable VGA text mode 3 by default)
- [X] PS/2 Keyboard driver: read scancodes, translate to keycodes/ASCII and pass to input subsystem
- [X] TTY subsystem: basic line discipline, buffering core utilities, tty driver
- [X] Console driver: write text to screen, with scrollback; interpret special characters like `^H`, `^M`, `^J`
- [X] Simple shell: run simple hardcoded commands
- [ ] Synchronization primitives: mask interrupts/save IRQ state, spinlocks (for multicore)
- [ ] Memory management: page fault handler, page table representation, page allocator, library functions for heap allocation
- [ ] Processes: TODO

See also [TODO.md](TODO.md).

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
