# The tty (terminal) subsystem

This page about the tty subsystem is also published [on my blog](https://lambdalambda.ninja/blog/54/).

### What is a terminal device?
The terminal is an abstraction used to manage I/O from a serial device. In its simplest form, let's assume we have a single keyboard, and a non-multitasking computer (only running one process at any time, and this process is in the foreground). Then, the terminal acts as the interface between the user and the process.

```
                  --< screen <--- (stdout)
                 /               \
user (master) <--    TERMINAL     --> process (slave)
                 \               /
                  --> keyboard -- (stdin)

```

This is all a terminal is. It has a "master" (interactive) and a "slave" (process) side, and two queues (one for input and one for output). The master side can have some additional functionality, such as sending signals to the slave. Like a pipe, both sides can write to the file: output on one side becomes input on the other.

In the old days, there were physical terminals (teletype machines, or tty), which were machines used to input data to and display output from mainframes, and the upper and lower lines were two pairs of physical wires (RX/TX). Now, we tend to abstract away terminals into Linux devices files, but the name tty persists.

### Virtualization
The picture becomes muddier once we add in some additional variables:

1. Multiple serial input devices?
2. Multiple process sessions for one screen?
3. Multiple processes or multiple devices attached to one terminal?
4. Emulated (non-physical) master device?

Addressing these issues:

1. We can simply allow multiple terminals. Each terminal is implemented in Linux as a device file (in devfs), and processes can read to/write from any of these files as if they were regular files.
2. This leads us to multiplex (virtualize) the screen. The _virtual terminal_ (vt) driver does this, multiplexing the screen over seven different physical terminals (`/dev/tty[1-7]`), each running their own session (shell). The keyboard is only "connected" to the active terminal session.
3. This leads us to multiplex the master and/or the slave side of a terminal. GNU `screen` is the prototypical example of this, turning the single-channel semantics into a broadcast semantics when multiple processes connect to the same terminal device.
4. We may want to create a terminal where the master side input does not come from a physical device (i.e., directly from the keyboard driver). Without going into too much depth, this is used in graphical modes like X.org to implement _terminal emulators_. In X.org, keyboard input from the physical terminal are turned into X events at `/dev/event*`, and sent to the foreground application. A terminal emulator like `xterm` may want to use this to emulate the master device of a terminal. To do this, it can use the _pseudoterminal_ (pty) driver to create a master-slave pair, such that events written to the master side emulate a physical device master.

### Updated diagram
On the left, we have what is more or less the pty driver stack; on the right we have the vt driver stack.

```
   Graphical mode/pseudoterminal       : Text mode/physical terminal
                                       :
+----------------+ +---------------+   :   +---------------+
| /usr/bin/xterm | | /usr/bin/bash |   :   | /usr/bin/bash |
|                | |               |   :   |               |
| terminal       | | process       |   :   | process       |
| emulator       | +---------------+   :   +---------------+
+----------------+    ^                :      ^
   ^                  |                :      |
   |                  v                :      v
   |               +--------------+    :   +--------------+
   |               | /dev/pts/3   |    :   | /dev/tty5    |
   |               |              |    :   |              |
   |               | pty tty      |    :   | vt tty       |
   |               | slave device |    :   | slave device |
   |               +--------------+    :   +--------------+
   |  **********      ^                :      ^
   | *xorg magic*     |                :      |
   |/ ********** \    v                :      v
   |            +--------------------------------------------------+
   |            | tty line discipline/other core editing utilities |
   |            +--------------------------------------------------+
   |              ^                    :      ^
   |             /                     :      |
   v            v                      :   +---------------+
+---------------+                      :   | keyboard IRQ  |
| /dev/ptmx     |                      :   |               |
|               |                      :   | vt tty        |
| pty tty       |                      :   | master device |
| master device |                      :   +---------------+
+---------------+                      :
```

I got tired of the ascii art around the "X.org magic" part. X.org manages a single virtual terminal (which we can think of as something like `/dev/tty7`, continuing the `vt` analogy). Keyboard events are signalled to the foreground process using `/dev/event*`, which are sent to the pty tty master device.

### Terminology
- **Terminal**: An abstract device that implements a bidirectional master/slave I/O channel.
- **Physical terminal (tty)**: Refers to a terminal device connected to a physical device, e.g., a serial port; now, "tty" is loosely used to refer to the subsystem in general and to terminal devices in general.
- **Virtual terminal (vt)**: Driver that multiplexes the text-mode display over multiple terminal devices, and performs text-mode rendering; provides the text-mode terminals usually found at `/dev/tty[1-6]` or <kbd>Ctrl</kbd>+<kbd>Alt</kbd>+<kbd>F1</kbd> through <kbd>Ctrl</kbd>+<kbd>Alt</kbd>+<kbd>F6</kbd>.
- **System console**: Usually refers to the text-mode terminal used for kernel space processes (i.e., calls to `printk()` get printed here, and this can usually be found at `/dev/tty1`).
- **Pseudoterminal (pty)**: Driver that provides a master/slave terminal device file pair (as opposed to a physical terminal, where the master device is usually implemented as a hardware driver); allows the use of a non-physical device as the terminal master, e.g., in userspace graphical mode.
- **Terminal emulator (e.g., `xterm`)**: A program running in graphical mode that provides an interface similar to a program running in a text-mode terminal using a pseudoterminal. In some ways, this is the graphical mode equivalent of a virtual terminal, and some people also call these programs virtual terminals.
- **Terminal multiplexer (e.g., GNU `screen`)**: Allow multiple processes to connect to a single terminal device (multi-channel terminal); i.e., reads and writes are broadcasted rather than being sent to a single process.
- **Shell** (e.g., `bash`): An interactive text-mode application that uses a terminal device for I/O; prompts user for commands to run and executes them, displaying their output. Shells aren't really related to this discussion, but sometimes conflated with the term "terminal".

### Use cases of tty
The tty interface can be used whenever you have a situation similar to the motivating scenario. Examples are the `ssh` and `telnet` commands, which are interactive sessions to some external computer. In this case, the slave process is not a local process, but rather a shell process (`ssh`) or some process writing to a port (`telnet`) on an remote machine.

### tty subsystem in the Linux source code
- `include/linux/tty.h`: Defines essential data structures related to a tty device, such as `struct tty_struct`.
- `include/linux/tty_driver.h`: Defines essential data structures for a tty driver implementer (such as vt and pty), such as `struct tty_driver` and `struct tty_operations`.
- `drivers/tty/vt/vt.c`: Defines a `struct tty_driver` for the six multiplexed text-mode physical terminal devices; provides the `/dev/tty*` slave device files.
- `drivers/tty/pty.c`: Defines a `struct tty_driver` for the `/dev/ptmx`/`/dev/pts/*` master/slave device files.

### Thought exercise: who does the rendering? And how do we echo?
This simple question stumped me for a little bit while I was trying to understand how the terminal works, and it was difficult to find an easy-to-swallow answer online. In text mode, who manages the rendering of the terminal text? And how do we manage the echo/noecho functionality (i.e., show or hide input from the master side)?

(Note: I'm not sure if the following answers are correct, these are just the conclusions that I've logically come to.)

Well, the master side is the one interacting with the system, and the one that wants to see the input and output to the process. The process doesn't need to render any of its I/O for to behave correctly; it is the human that needs it. So the master side manages any rendering, and can decide to turn off any rendering if need be.

In the case of the system console, the master side of the `vt` driver renders data (using the VGA framebuffer) from the output queue when output is generated from the slave side. In the case of a graphical-mode terminal emulator, the master side (e.g., `xterm` connected to `/dev/ptmx`) renders likewise (albeit using X.org drawing commands).

Then, what about displaying keyboard data from the master side? (I.e., how do we implement echoing?) We can do this by "echoing" (copying) the input stream onto the output queue. This is a change at the `tty` device level rather than the `vt`/`ptmx` driver level. (In Linux, I do not see any handling of echo in the `vt`/`ptmx` drivers, so I assume that it is performed in the `tty` device layer in this way.)

### Special editing semantics
While tty devices are mostly a "dumb" device, there are a few editing semantics that make sense for the interactive asymmetric terminal interface. This special behavior is transmitted using special (ascii) characters or character sequences from the keyboard, such as <kbd>Ctrl</kbd>+<kbd>C</kbd> (ascii code 3) or <kbd>Bksp</kbd> (ascii code 8). I won't go into these in detail here, these are explained well in other articles, and I don't find them terribly useful to understanding the core of the tty subsystem.

1. **"Line discipline"**: This allows for some editing of the buffer by using special characters such as <kbd>Bksp</kbd>.
2. **Signals**: The master side can send POSIX signals (e.g., SIGHUP, SIGTERM) to the process using special characters such as <kbd>Ctrl</kbd>+<kbd>C</kbd>.
3. **VT100 display codes** (a.k.a., ANSI escape codes): Used to manipulate the rendering of the output, such as moving the cursor, coloring the screen, etc. There are some newer standards, e.g., VT220.

### Resources
Wikipedia links to the major terminology words are also recommended but not included below, because you can find those yourself.

- [The TTY demystified](http://www.linusakesson.net/programming/tty/): Highly recommended by many people. Provides a general overview of the tty subsystem, and especially talks about line discipline and signals.
- [What are the responsibilities of each Pseudo-Terminal (PTY) component (software, master side, slave side)?](https://unix.stackexchange.com/q/117981/307410): Many fantastic answers about tty in general, line discipline, and the pty master/slave relationship.
- [TTY questions](https://www.reddit.com/r/osdev/comments/hgzg6k/comment/fwabf1j/?utm_source=reddit&utm_medium=web2x&context=3): Some considerations when designing a tty implementation.
- [Linux keyboard/terminal/kernel input buffer](https://stackoverflow.com/q/69198577/2397327): How large is the tty buffer? This suggests `N_TTY_BUF_SIZE == 4096` is the maximum buffer size (see `drivers/tty/n_tty.c`).
- [Linux keyboard/terminal/kernel input buffer](https://stackoverflow.com/q/24024985/2397327): Motivates a terminal multiplexer like GNU `screen`.
- [Linux keyboard/terminal/kernel input buffer](https://stackoverflow.com/q/48627344/2397327): Another Internet user trying to figure out how the tty subsystem works.
- [termios(3) — Linux manual page](https://man7.org/linux/man-pages/man3/termios.3.html): Interface to set terminal device settings, such as echo.
- [How does input echoing work in a Linux terminal?](https://stackoverflow.com/a/23317250/2397327): Summarizes the many responsibilities of the tty subsystem.
- [How to read/write to tty* device?](https://unix.stackexchange.com/a/138390/307410): Provides some examples of how to use `/dev/tty` files.
- [What are pseudo terminals (pty/tty)?](https://unix.stackexchange.com/q/21147/307410): Gives a simple explanation of pty.
- [What is the purpose of the controlling terminal?](https://unix.stackexchange.com/q/404555/307410): Explains the concept of a controlling terminal.
- [Does keyboard input always go through a controlling terminal?](https://unix.stackexchange.com/a/351064/307410): Explains some other implications of a controlling terminal.
- [Port of the Limine bootloader terminal](https://github.com/V01D-NULL/limine-terminal-port): Isolated terminal implementation from the Limine bootloader, with basic text-mode rendering.
- [OSDev: Terminals](https://wiki.osdev.org/Terminals)
- [Linux: Difference between /dev/console , /dev/tty and /dev/tty0](https://unix.stackexchange.com/q/60641/307410)
- [bionic (4) vt.4freebsd.gz](https://manpages.ubuntu.com/manpages/bionic/man4/vt.4freebsd.html): `vt` manpage for BSD
