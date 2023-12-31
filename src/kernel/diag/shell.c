#include "diag/shell.h"

#include "common/libc.h"
#include "common/util.h"
#include "diag/mm.h"
#include "drivers/console.h"
#include "drivers/term.h"
#include "mem/phys.h"
#include "sched/sched.h"
#include "test/test.h"

#include "proc/process.h" // for proc_jump_userspace

#define SHELL_INPUT_BUF_SZ 4095

static struct term_driver *_term_driver;
static char _shell_input_buf[SHELL_INPUT_BUF_SZ + 1];
static size_t _shell_input_size;
static const char *_shell_prompt_string = "$ ";

/**
 * Display shell prompt.
 */
void _shell_prompt(void) { printf(_shell_prompt_string); }

/**
 * Enqueue a byte onto the prompt. Do a very rough treatment of ^M and ^H.
 */
void _shell_enqueue_byte(char c) {
  if (c == '\b') {
    if (_shell_input_size) {
      --_shell_input_size;
    }
  } else if (c == '\r') {
    _shell_input_size = 0;
  } else if (_shell_input_size < SHELL_INPUT_BUF_SZ) {
    _shell_input_buf[_shell_input_size++] = c;
  }
}

void _shell_dispatch(const char *cmd) {
  if (!strncmp(cmd, "help", SHELL_INPUT_BUF_SZ)) {
    printf("\rHelp menu:\r\n");
  } else if (!strncmp(cmd, "mm", SHELL_INPUT_BUF_SZ)) {
    print_mm();
  } else if (!strncmp(cmd, "phys", SHELL_INPUT_BUF_SZ)) {
    phys_mem_print_stats();
  } else if (!strncmp(cmd, "pa", SHELL_INPUT_BUF_SZ)) {
    // Allocate a random page. For testing purposes.
    printf("\rret=%lx\r\n", phys_alloc_page());
  } else if (!strncmp(cmd, "rt", strlen("rt"))) {
    // Get argument 2. Need to write a vector-like library.
    // TODO(jlam55555): This is pretty unsafe.
    run_tests(cmd + strlen("rt") + 1);
  } else {
    printf("\rUnknown command.\r\n");
  }
}

/**
 * Respond to the input.
 *
 * TODO(jlam55555): Note that the input string cannot have ^J, ^M, nor ^H
 *    characters due to their special treatment at the moment. Also, it may
 *    have null characters within, which may force us not to use printf.
 */
void _shell_handle_input(void) {
  // Null-terminate string.
  _shell_input_buf[_shell_input_size] = 0;

  // Dispatch command.
  _shell_dispatch(_shell_input_buf);

  // Reset terminal input.
  _shell_input_size = 0;
}

static void _foo(void) {
  // Trigger a GP fault:
  /* __asm__ volatile("cli"); */

  // Still doesn't work because writing to the terminal device requires a
  // privileged command. Sad. Will implement syscalls soon.
  /* printf("Made it to userspace! Woohoo!\r\n"); */

  // Syscall interface is not set up yet.
  __asm__ volatile("syscall");

  for (;;) {
  }
}

void shell_init() {
  // Need to re-enable interrupts once the task is created since we cli upon
  // entering the scheduler.
  //
  // TODO(jlam55555): Move this to a helper function so we don't need this at
  // the beginning of each thread.
  __asm__ volatile("sti");

  _term_driver = get_default_term_driver();
  _shell_input_size = 0;
  _shell_prompt();

  // Jump into userspace.
  proc_jump_userspace(_foo);

  // Yield task; nothing to do for now.
  /* for (;;) { */
  /*   shell_on_interrupt(); */
  /*   schedule(); */
  /* } */
}

void shell_on_interrupt() {
  static char buf[TERM_BUF_SIZE];
  size_t bytes;

  // TODO(jlam55555): Need to make this a loop in case an interrupt
  //     arrives in the middle of processing an event.
  // TODO(jlam55555): Need to make the ringbuffer interrupt-safe.
  if (!(bytes = _term_driver->slave_read(_term_driver->dev, buf,
                                         SHELL_INPUT_BUF_SZ))) {
    return;
  }

  // Handle input.
  for (size_t i = 0; i < bytes; ++i) {
    if (buf[i] == '\n') {
      _shell_handle_input();
      _shell_prompt();
    } else {
      _shell_enqueue_byte(buf[i]);
    }
  }
}
