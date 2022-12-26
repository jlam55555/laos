# laos -- LAm Operating System

A personal study on operating systems, with help from the OSDev Wiki. A long-term weekend side project.

### Quick start

Make sure to have [QEMU][qemu] and [GNU `make`][make] installed.

```bash
# Fetch Limine dependency
git submodule init
git submodule update

# Build and run either iso or hdd
make run_iso
make run_hdd
```

See the [Makefile](./Makefile) for more possible build targets.

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
